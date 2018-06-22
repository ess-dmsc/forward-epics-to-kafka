#include "Forwarder.h"
#include "CommandHandler.h"
#include "Converter.h"
#include "Stream.h"
#include "Timer.h"
#include "helper.h"
#include "logger.h"
#include <EpicsClient/EpicsClientInterface.h>
#include <EpicsClient/EpicsClientMonitor.h>
#include <EpicsClient/EpicsClientRandom.h>
#include <nlohmann/json.hpp>
#include <sys/types.h>
#ifdef _MSC_VER
#include "process.h"
#define getpid _getpid
#else
#include <unistd.h>
#endif
#include "CURLReporter.h"

namespace Forwarder {

static bool isStopDueToSignal(ForwardingRunState Flag) {
  return static_cast<int>(Flag) &
         static_cast<int>(ForwardingRunState::STOP_DUE_TO_SIGNAL);
}

// Little helper
static KafkaW::BrokerSettings make_broker_opt(MainOpt const &opt) {
  KafkaW::BrokerSettings ret = opt.broker_opt;
  ret.Address = opt.brokers_as_comma_list();
  return ret;
}

using ulock = std::unique_lock<std::mutex>;

/// \class Main
/// \brief Main program entry class.
Forwarder::Forwarder(MainOpt &opt)
    : main_opt(opt), kafka_instance_set(InstanceSet::Set(make_broker_opt(opt))),
      conversion_scheduler(this) {

  for (size_t i = 0; i < opt.MainSettings.ConversionThreads; ++i) {
    conversion_workers.emplace_back(make_unique<ConversionWorker>(
        &conversion_scheduler,
        static_cast<uint32_t>(opt.MainSettings.ConversionWorkerQueueSize)));
  }

  bool use_config = true;
  if (main_opt.MainSettings.BrokerConfig.topic.empty()) {
    LOG(3, "Name for configuration topic is empty");
    use_config = false;
  }
  if (main_opt.MainSettings.BrokerConfig.host.empty()) {
    LOG(3, "Host for configuration topic broker is empty");
    use_config = false;
  }
  if (use_config) {
    KafkaW::BrokerSettings bopt;
    bopt.ConfigurationStrings["group.id"] =
        fmt::format("forwarder-command-listener--pid{}", getpid());
    config_listener.reset(
        new Config::Listener{bopt, main_opt.MainSettings.BrokerConfig});
  }
  createPVUpdateTimerIfRequired();
  createFakePVUpdateTimerIfRequired();

  for (auto &Stream : main_opt.MainSettings.StreamsInfo) {
    try {
      addMapping(Stream);
    } catch (std::exception &e) {
      LOG(4, "Could not add mapping: {}  {}", Stream.Name, e.what());
    }
  }

  curl = ::make_unique<CURLReporter>();
  if (!main_opt.MainSettings.StatusReportURI.host.empty()) {
    KafkaW::BrokerSettings BrokerSettings;
    BrokerSettings.Address = main_opt.MainSettings.StatusReportURI.host_port;
    status_producer = std::make_shared<KafkaW::Producer>(BrokerSettings);
    status_producer_topic = ::make_unique<KafkaW::ProducerTopic>(
        status_producer, main_opt.MainSettings.StatusReportURI.topic);
  }
}

Forwarder::~Forwarder() {
  LOG(7, "~Main");
  streams.streams_clear();
  conversion_workers_clear();
  converters_clear();
  InstanceSet::clear();
}

void Forwarder::createPVUpdateTimerIfRequired() {
  if (main_opt.PeriodMS > 0) {
    auto Interval = std::chrono::milliseconds(main_opt.PeriodMS);
    std::shared_ptr<Sleeper> IntervalSleeper = std::make_shared<RealSleeper>();
    PVUpdateTimer = ::make_unique<Timer>(Interval, IntervalSleeper);
  }
}

void Forwarder::createFakePVUpdateTimerIfRequired() {
  if (main_opt.FakePVPeriodMS > 0) {
    auto Interval = std::chrono::milliseconds(main_opt.FakePVPeriodMS);
    std::shared_ptr<Sleeper> IntervalSleeper = std::make_shared<RealSleeper>();
    GenerateFakePVUpdateTimer = ::make_unique<Timer>(Interval, IntervalSleeper);
  }
}

int Forwarder::conversion_workers_clear() {
  CLOG(7, 1, "Main::conversion_workers_clear()  begin");
  std::unique_lock<std::mutex> lock(conversion_workers_mx);
  if (!conversion_workers.empty()) {
    for (auto &x : conversion_workers) {
      x->stop();
    }
    conversion_workers.clear();
  }
  CLOG(7, 1, "Main::conversion_workers_clear()  end");
  return 0;
}

int Forwarder::converters_clear() {
  if (!conversion_workers.empty()) {
    std::unique_lock<std::mutex> lock(converters_mutex);
    conversion_workers.clear();
  }
  return 0;
}

std::unique_lock<std::mutex> Forwarder::get_lock_streams() {
  return std::unique_lock<std::mutex>(streams_mutex);
}

std::unique_lock<std::mutex> Forwarder::get_lock_converters() {
  return std::unique_lock<std::mutex>(converters_mutex);
}

/// \brief Main program loop.
///
/// Start conversion worker threads, poll for commands from Kafka.
/// When stop flag raised, clear all workers and streams.
void Forwarder::forward_epics_to_kafka() {
  using CLK = std::chrono::steady_clock;
  using MS = std::chrono::milliseconds;
  auto Dt = MS(main_opt.MainSettings.MainPollInterval);
  auto t_lf_last = CLK::now();
  auto t_status_last = CLK::now();
  ConfigCB config_cb(*this);
  {
    std::unique_lock<std::mutex> lock(conversion_workers_mx);
    for (auto &x : conversion_workers) {
      x->start();
    }
  }

  if (PVUpdateTimer != nullptr)
    PVUpdateTimer->start();

  if (GenerateFakePVUpdateTimer != nullptr)
    GenerateFakePVUpdateTimer->start();

  while (ForwardingRunFlag.load() == ForwardingRunState::RUN) {
    auto do_stats = false;
    auto t1 = CLK::now();
    if (t1 - t_lf_last > MS(2000)) {
      if (config_listener) {
        config_listener->poll(config_cb);
      }
      streams.check_stream_status();
      t_lf_last = t1;
      do_stats = true;
    }
    kafka_instance_set->poll();

    auto t2 = CLK::now();
    auto dt = std::chrono::duration_cast<MS>(t2 - t1);
    if (t2 - t_status_last > MS(3000)) {
      if (status_producer_topic) {
        report_status();
      }
      t_status_last = t2;
    }
    if (do_stats) {
      kafka_instance_set->log_stats();
      report_stats(dt.count());
    }
    if (dt >= Dt) {
      CLOG(3, 1, "slow main loop: {}", dt.count());
    } else {
      std::this_thread::sleep_for(Dt - dt);
    }
  }
  if (isStopDueToSignal(ForwardingRunFlag.load())) {
    LOG(6, "Forwarder stopping due to signal.");
  }
  LOG(6, "Main::forward_epics_to_kafka shutting down");
  conversion_workers_clear();
  streams.streams_clear();

  if (PVUpdateTimer != nullptr) {
    PVUpdateTimer->triggerStop();
    PVUpdateTimer->waitForStop();
  }

  if (GenerateFakePVUpdateTimer != nullptr) {
    GenerateFakePVUpdateTimer->triggerStop();
    GenerateFakePVUpdateTimer->waitForStop();
  }

  LOG(6, "ForwardingStatus::STOPPED");
  forwarding_status.store(ForwardingStatus::STOPPED);
}

void Forwarder::report_status() {
  using nlohmann::json;
  auto Status = json::object();
  auto Streams = json::array();
  for (auto const &Stream : streams.get_streams()) {
    Streams.push_back(Stream->status_json());
  }
  Status["streams"] = Streams;
  auto StatusString = Status.dump();
  auto StatusStringSize = StatusString.size();
  if (StatusStringSize > 1000) {
    auto StatusStringShort =
        StatusString.substr(0, 1000) +
        fmt::format(" ... {} chars total ...", StatusStringSize);
    LOG(7, "status: {}", StatusStringShort);
  } else {
    LOG(7, "status: {}", StatusString);
  }
  status_producer_topic->produce((KafkaW::uchar *)StatusString.c_str(),
                                 StatusString.size());
}

void Forwarder::report_stats(int dt) {
  fmt::MemoryWriter influxbuf;
  auto m1 = g__total_msgs_to_kafka.load();
  auto m2 = m1 / 1000;
  m1 = m1 % 1000;
  uint64_t b1 = g__total_bytes_to_kafka.load();
  auto b2 = b1 / 1024;
  b1 %= 1024;
  auto b3 = b2 / 1024;
  b2 %= 1024;
  LOG(6, "dt: {:4}  m: {:4}.{:03}  b: {:3}.{:03}.{:03}", dt, m2, m1, b3, b2,
      b1);
  if (CURLReporter::HaveCURL && !main_opt.InfluxURI.empty()) {
    int i1 = 0;
    for (auto &s : kafka_instance_set->stats_all()) {
      auto &m1 = influxbuf;
      m1.write("forward-epics-to-kafka,hostname={},set={}",
               main_opt.Hostname.data(), i1);
      m1.write(" produced={}", s.produced);
      m1.write(",produce_fail={}", s.produce_fail);
      m1.write(",local_queue_full={}", s.local_queue_full);
      m1.write(",produce_cb={}", s.produce_cb);
      m1.write(",produce_cb_fail={}", s.produce_cb_fail);
      m1.write(",poll_served={}", s.poll_served);
      m1.write(",msg_too_large={}", s.msg_too_large);
      m1.write(",produced_bytes={}", double(s.produced_bytes));
      m1.write(",outq={}", s.out_queue);
      m1.write("\n");
      ++i1;
    }
    {
      auto lock = get_lock_converters();
      LOG(6, "N converters: {}", converters.size());
      i1 = 0;
      for (auto &c : converters) {
        auto stats = c.second.lock()->stats();
        auto &m1 = influxbuf;
        m1.write("forward-epics-to-kafka,hostname={},set={}",
                 main_opt.Hostname.data(), i1);
        int i2 = 0;
        for (auto x : stats) {
          if (i2 > 0) {
            m1.write(",");
          } else {
            m1.write(" ");
          }
          m1.write("{}={}", x.first, x.second);
          ++i2;
        }
        m1.write("\n");
        ++i1;
      }
    }
    curl->send(influxbuf, main_opt.InfluxURI);
  }
}

void Forwarder::pushConverterToStream(ConverterSettings const &ConverterInfo,
                                      std::shared_ptr<Stream> &Stream) {

  // Check schema exists
  auto r1 = main_opt.schema_registry.items().find(ConverterInfo.Schema);
  if (r1 == main_opt.schema_registry.items().end()) {
    throw MappingAddException(fmt::format(
        "Cannot handle flatbuffer schema id {}", ConverterInfo.Schema));
  }

  URI Uri;
  if (!main_opt.MainSettings.Brokers.empty()) {
    Uri = main_opt.MainSettings.Brokers[0];
  }

  URI TopicURI;
  if (!Uri.host.empty()) {
    TopicURI.host = Uri.host;
  }

  if (Uri.port != 0) {
    TopicURI.port = Uri.port;
  }
  TopicURI.parse(ConverterInfo.Topic);

  Converter::sptr ConverterShared;
  if (!ConverterInfo.Name.empty()) {
    auto Lock = get_lock_converters();
    auto ConverterIt = converters.find(ConverterInfo.Name);
    if (ConverterIt != converters.end()) {
      ConverterShared = ConverterIt->second.lock();
      if (!ConverterShared) {
        ConverterShared = Converter::create(main_opt.schema_registry,
                                            ConverterInfo.Schema, main_opt);
        converters[ConverterInfo.Name] =
            std::weak_ptr<Converter>(ConverterShared);
      }
    } else {
      ConverterShared = Converter::create(main_opt.schema_registry,
                                          ConverterInfo.Schema, main_opt);
      converters[ConverterInfo.Name] =
          std::weak_ptr<Converter>(ConverterShared);
    }
  } else {
    ConverterShared = Converter::create(main_opt.schema_registry,
                                        ConverterInfo.Schema, main_opt);
  }
  if (!ConverterShared) {
    throw MappingAddException("Cannot create a converter");
  }
  Stream->converter_add(*kafka_instance_set, ConverterShared, TopicURI);
}

void Forwarder::addMapping(StreamSettings const &StreamInfo) {
  std::unique_lock<std::mutex> lock(streams_mutex);
  try {
    ChannelInfo ChannelInfo{StreamInfo.EpicsProtocol, StreamInfo.Name};
    std::shared_ptr<EpicsClient::EpicsClientInterface> Client;
    if (GenerateFakePVUpdateTimer != nullptr) {
      Client = addStream<EpicsClient::EpicsClientRandom>(ChannelInfo);
      auto RandomClient =
          std::static_pointer_cast<EpicsClient::EpicsClientRandom>(Client);
      GenerateFakePVUpdateTimer->addCallback(
          [RandomClient]() { RandomClient->generateFakePVUpdate(); });
    } else
      Client = addStream<EpicsClient::EpicsClientMonitor>(ChannelInfo);
    if (PVUpdateTimer != nullptr) {
      auto PeriodicClient =
          std::static_pointer_cast<EpicsClient::EpicsClientMonitor>(Client);
      PVUpdateTimer->addCallback(
          [PeriodicClient]() { PeriodicClient->emitCachedValue(); });
    }
  } catch (std::runtime_error &e) {
    std::throw_with_nested(MappingAddException("Cannot add stream"));
  }

  auto Stream = streams.back();

  for (auto &Converter : StreamInfo.Converters) {
    pushConverterToStream(Converter, Stream);
  }
}

template <typename T>
std::shared_ptr<T> Forwarder::addStream(ChannelInfo &ChannelInfo) {
  auto PVUpdateRing = std::make_shared<
      moodycamel::ConcurrentQueue<std::shared_ptr<FlatBufs::EpicsPVUpdate>>>();
  auto client = std::make_shared<T>(ChannelInfo, PVUpdateRing);
  auto EpicsClientInterfacePtr =
      std::static_pointer_cast<EpicsClient::EpicsClientInterface>(client);
  auto stream = std::make_shared<Stream>(ChannelInfo, EpicsClientInterfacePtr,
                                         PVUpdateRing);
  streams.add(stream);
  return client;
}

std::atomic<uint64_t> g__total_msgs_to_kafka{0};
std::atomic<uint64_t> g__total_bytes_to_kafka{0};

void Forwarder::raiseForwardingFlag(ForwardingRunState ToBeRaised) {
  while (true) {
    auto Expect = ForwardingRunFlag.load();
    auto Desired = static_cast<ForwardingRunState>(
        static_cast<int>(Expect) | static_cast<int>(ToBeRaised));
    if (ForwardingRunFlag.compare_exchange_weak(Expect, Desired)) {
      break;
    }
  }
}

void Forwarder::stopForwarding() {
  raiseForwardingFlag(ForwardingRunState::STOP);
}

void Forwarder::stopForwardingDueToSignal() {
  raiseForwardingFlag(ForwardingRunState::STOP_DUE_TO_SIGNAL);
}
} // namespace Forwarder
