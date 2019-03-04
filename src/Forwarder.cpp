#include "Forwarder.h"
#include "CommandHandler.h"
#include "Converter.h"
#include "KafkaOutput.h"
#include "Stream.h"
#include "Timer.h"
#include "helper.h"
#include "logger.h"
#include <EpicsClient/EpicsClientInterface.h>
#include <EpicsClient/EpicsClientMonitor.h>
#include <EpicsClient/EpicsClientRandom.h>
#include <algorithm>
#include <nlohmann/json.hpp>
#include <sys/types.h>
#ifdef _MSC_VER
std::vector<char> getHostname() {
  std::vector<char> Hostname;
  return Hostname;
}
#else
#include <unistd.h>
std::vector<char> getHostname() {
  std::vector<char> Hostname;
  Hostname.resize(256);
  gethostname(Hostname.data(), Hostname.size());
  if (Hostname.back() != 0) {
    // likely an error
    Hostname.back() = 0;
  }
  return Hostname;
}

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

/// Main program entry class.
Forwarder::Forwarder(MainOpt &opt)
    : main_opt(opt), KafkaInstanceSet(InstanceSet::Set(make_broker_opt(opt))),
      conversion_scheduler(this) {

  for (size_t i = 0; i < opt.MainSettings.ConversionThreads; ++i) {
    conversion_workers.emplace_back(make_unique<ConversionWorker>(
        &conversion_scheduler,
        static_cast<uint32_t>(opt.MainSettings.ConversionWorkerQueueSize)));
  }

  bool use_config = true;
  if (main_opt.MainSettings.BrokerConfig.Topic.empty()) {
    LOG(Sev::Error, "Name for configuration topic is empty");
    use_config = false;
  }
  if (main_opt.MainSettings.BrokerConfig.HostPort.empty()) {
    LOG(Sev::Error, "Host for configuration topic broker is empty");
    use_config = false;
  }
  if (use_config) {
    KafkaW::BrokerSettings bopt;
    bopt.Address = main_opt.MainSettings.BrokerConfig.HostPort;
    bopt.PollTimeoutMS = 0;
    auto NewConsumer = make_unique<KafkaW::Consumer>(bopt);
    config_listener.reset(new Config::Listener{
        main_opt.MainSettings.BrokerConfig, std::move(NewConsumer)});
  }
  createPVUpdateTimerIfRequired();
  createFakePVUpdateTimerIfRequired();

  for (auto &Stream : main_opt.MainSettings.StreamsInfo) {
    try {
      addMapping(Stream);
    } catch (std::exception &e) {
      LOG(Sev::Warning, "Could not add mapping: {}  {}", Stream.Name, e.what());
    }
  }

  if (!main_opt.MainSettings.StatusReportURI.HostPort.empty()) {
    KafkaW::BrokerSettings BrokerSettings;
    BrokerSettings.Address = main_opt.MainSettings.StatusReportURI.HostPort;
    status_producer = std::make_shared<KafkaW::Producer>(BrokerSettings);
    status_producer_topic = ::make_unique<KafkaW::ProducerTopic>(
        status_producer, main_opt.MainSettings.StatusReportURI.Topic);
  }
}

Forwarder::~Forwarder() {
  LOG(Sev::Debug, "~Main");
  streams.clearStreams();
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
  LOG(Sev::Debug, "Main::conversion_workers_clear()  begin");
  std::lock_guard<std::mutex> lock(conversion_workers_mx);
  if (!conversion_workers.empty()) {
    for (auto &x : conversion_workers) {
      x->stop();
    }
    conversion_workers.clear();
  }
  LOG(Sev::Debug, "Main::conversion_workers_clear()  end");
  return 0;
}

int Forwarder::converters_clear() {
  if (!conversion_workers.empty()) {
    auto lock = get_lock_converters();
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

/// Main program loop.
///
/// Start conversion worker threads, poll for commands from Kafka.
/// When stop flag raised, clear all workers and streams.
void Forwarder::forward_epics_to_kafka() {
  using CLK = std::chrono::steady_clock;
  using MS = std::chrono::milliseconds;
  auto Dt = MS(main_opt.MainSettings.MainPollInterval);
  auto t_lf_last = CLK::now();
  auto t_status_last = CLK::now() - MS(4000);
  ConfigCB config_cb(*this);
  {
    std::lock_guard<std::mutex> lock(conversion_workers_mx);
    for (auto &x : conversion_workers) {
      x->start();
    }
  }

  if (PVUpdateTimer != nullptr) {
    PVUpdateTimer->start();
  }

  if (GenerateFakePVUpdateTimer != nullptr) {
    GenerateFakePVUpdateTimer->start();
  }

  while (ForwardingRunFlag.load() == ForwardingRunState::RUN) {
    auto do_stats = false;
    auto t1 = CLK::now();
    if (t1 - t_lf_last > MS(2000)) {
      if (config_listener) {
        config_listener->poll(config_cb);
      }
      streams.checkStreamStatus();
      t_lf_last = t1;
      do_stats = true;
    }
    KafkaInstanceSet->poll();

    auto t2 = CLK::now();
    auto dt = std::chrono::duration_cast<MS>(t2 - t1);
    if (t2 - t_status_last > MS(3000)) {
      if (status_producer_topic) {
        report_status();
      }
      t_status_last = t2;
    }
    if (do_stats) {
      KafkaInstanceSet->log_stats();
      report_stats(dt.count());
    }
    if (dt >= Dt) {
      LOG(Sev::Error, "slow main loop: {}", dt.count());
    } else {
      std::this_thread::sleep_for(Dt - dt);
    }
  }
  if (isStopDueToSignal(ForwardingRunFlag.load())) {
    LOG(Sev::Info, "Forwarder stopping due to signal.");
  }
  LOG(Sev::Info, "Main::forward_epics_to_kafka shutting down");
  conversion_workers_clear();
  streams.clearStreams();

  if (PVUpdateTimer != nullptr) {
    PVUpdateTimer->triggerStop();
    PVUpdateTimer->waitForStop();
  }

  if (GenerateFakePVUpdateTimer != nullptr) {
    GenerateFakePVUpdateTimer->triggerStop();
    GenerateFakePVUpdateTimer->waitForStop();
  }

  LOG(Sev::Info, "ForwardingStatus::STOPPED");
  forwarding_status.store(ForwardingStatus::STOPPED);
}

void Forwarder::report_status() {
  using nlohmann::json;
  auto Status = json::object();
  Status["service_id"] = main_opt.MainSettings.ServiceID;
  auto Streams = json::array();
  auto StreamVector = streams.getStreams();
  std::transform(StreamVector.cbegin(), StreamVector.cend(),
                 std::back_inserter(Streams),
                 [](const std::shared_ptr<Stream> &CStream) {
                   return CStream->getStatusJson();
                 });
  Status["streams"] = Streams;
  auto StatusString = Status.dump();
  auto StatusStringSize = StatusString.size();
  if (StatusStringSize > 1000) {
    auto StatusStringShort =
        StatusString.substr(0, 1000) +
        fmt::format(" ... {} chars total", StatusStringSize);
    LOG(Sev::Debug, "status: {}", StatusStringShort);
  } else {
    LOG(Sev::Debug, "status: {}", StatusString);
  }
  status_producer_topic->produce((unsigned char *)StatusString.c_str(),
                                 StatusString.size());
}

void Forwarder::report_stats(int dt) {
  fmt::memory_buffer StatsBuffer;
  auto m1 = g__total_msgs_to_kafka.load();
  auto m2 = m1 / 1000;
  m1 = m1 % 1000;
  uint64_t b1 = g__total_bytes_to_kafka.load();
  auto b2 = b1 / 1024;
  b1 %= 1024;
  auto b3 = b2 / 1024;
  b2 %= 1024;
  LOG(Sev::Info, "dt: {:4}  m: {:4}.{:03}  b: {:3}.{:03}.{:03}", dt, m2, m1, b3,
      b2, b1);
  if (CURLReporter::HaveCURL && !main_opt.InfluxURI.empty()) {
    std::vector<char> Hostname = getHostname();
    int i1 = 0;
    for (auto &s : KafkaInstanceSet->getStatsForAllProducers()) {
      fmt::format_to(StatsBuffer, "forward-epics-to-kafka,hostname={},set={}",
                     Hostname.data(), i1);
      fmt::format_to(StatsBuffer, " produced={}", s.produced);
      fmt::format_to(StatsBuffer, ",produce_fail={}", s.produce_fail);
      fmt::format_to(StatsBuffer, ",local_queue_full={}", s.local_queue_full);
      fmt::format_to(StatsBuffer, ",produce_cb={}", s.produce_cb);
      fmt::format_to(StatsBuffer, ",produce_cb_fail={}", s.produce_cb_fail);
      fmt::format_to(StatsBuffer, ",poll_served={}", s.poll_served);
      fmt::format_to(StatsBuffer, ",msg_too_large={}", s.msg_too_large);
      fmt::format_to(StatsBuffer, ",produced_bytes={}",
                     double(s.produced_bytes));
      fmt::format_to(StatsBuffer, ",outq={}", s.out_queue);
      fmt::format_to(StatsBuffer, "\n");
      ++i1;
    }
    {
      auto lock = get_lock_converters();
      LOG(Sev::Info, "N converters: {}", converters.size());
      i1 = 0;
      for (auto &c : converters) {
        auto stats = c.second.lock()->stats();
        fmt::format_to(StatsBuffer, "forward-epics-to-kafka,hostname={},set={}",
                       Hostname.data(), i1);
        int i2 = 0;
        for (auto x : stats) {
          if (i2 > 0) {
            fmt::format_to(StatsBuffer, ",");
          } else {
            fmt::format_to(StatsBuffer, " ");
          }
          fmt::format_to(StatsBuffer, "{}={}", x.first, x.second);
          ++i2;
        }
        fmt::format_to(StatsBuffer, "\n");
        ++i1;
      }
    }
    CURLReporter::send(StatsBuffer, main_opt.InfluxURI);
  }
}

URI Forwarder::createTopicURI(ConverterSettings const &ConverterInfo) {
  URI BrokerURI;
  if (!main_opt.MainSettings.Brokers.empty()) {
    BrokerURI = main_opt.MainSettings.Brokers[0];
  }

  URI TopicURI;
  if (!BrokerURI.HostPort.empty()) {
    TopicURI.HostPort = BrokerURI.HostPort;
  }

  if (BrokerURI.Port != 0) {
    TopicURI.Port = BrokerURI.Port;
  }
  try {
    TopicURI.parse(ConverterInfo.Topic);
  } catch (std::runtime_error &e) {
    throw MappingAddException(
        fmt::format("Invalid topic {} in converter, not added to stream. May "
                    "require broker and/or host slashes.",
                    ConverterInfo.Topic));
  }
  return TopicURI;
}

void Forwarder::pushConverterToStream(ConverterSettings const &ConverterInfo,
                                      std::shared_ptr<Stream> &Stream) {

  // Check schema exists
  auto r1 = FlatBufs::SchemaRegistry::items().find(ConverterInfo.Schema);
  if (r1 == FlatBufs::SchemaRegistry::items().end()) {
    throw MappingAddException(fmt::format(
        "Cannot handle flatbuffer schema id {}", ConverterInfo.Schema));
  }

  URI TopicURI = createTopicURI(ConverterInfo);

  std::shared_ptr<Converter> ConverterShared;
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

  // Create a conversion path then add it
  auto Topic = KafkaInstanceSet->SetUpProducerTopic(std::move(TopicURI));
  auto cp = ::make_unique<ConversionPath>(
      std::move(ConverterShared), ::make_unique<KafkaOutput>(std::move(Topic)));

  Stream->addConverter(std::move(cp));
}

void Forwarder::addMapping(StreamSettings const &StreamInfo) {
  auto lock = get_lock_streams();
  try {
    ChannelInfo ChannelInfo{StreamInfo.EpicsProtocol, StreamInfo.Name};
    std::shared_ptr<Stream> Stream;
    if (GenerateFakePVUpdateTimer != nullptr) {
      auto UpdateQueue = std::make_shared<moodycamel::ConcurrentQueue<
          std::shared_ptr<FlatBufs::EpicsPVUpdate>>>();
      auto EpicsClient = std::make_shared<EpicsClient::EpicsClientRandom>(
          ChannelInfo, UpdateQueue);
      Stream = findOrAddStream<EpicsClient::EpicsClientRandom>(
          ChannelInfo, EpicsClient, UpdateQueue);
      auto Client = Stream->getEpicsClient();
      auto RandomClient =
          dynamic_cast<EpicsClient::EpicsClientRandom *>(Client.get());
      if (RandomClient) {
        GenerateFakePVUpdateTimer->addCallback(
            [Client, RandomClient]() { RandomClient->generateFakePVUpdate(); });
      }
    } else {
      auto UpdateQueue = std::make_shared<moodycamel::ConcurrentQueue<
          std::shared_ptr<FlatBufs::EpicsPVUpdate>>>();
      auto EpicsClient = std::make_shared<EpicsClient::EpicsClientMonitor>(
          ChannelInfo, UpdateQueue);
      if (StreamInfo.Converters.size() > 0) {
        auto const &Converter = StreamInfo.Converters.at(0);
        auto ConnectionStatusProducer =
            KafkaInstanceSet->SetUpProducerTopic(URI(Converter.Topic));
        EpicsClient->ConnectionStatusProducer =
            make_unique<KafkaW::ProducerTopic>(
                std::move(ConnectionStatusProducer));
      }
      Stream = findOrAddStream<EpicsClient::EpicsClientMonitor>(
          ChannelInfo, EpicsClient, UpdateQueue);
    }

    if (PVUpdateTimer != nullptr) {
      auto Client = Stream->getEpicsClient();
      auto PeriodicClient =
          dynamic_cast<EpicsClient::EpicsClientMonitor *>(Client.get());
      PVUpdateTimer->addCallback(
          [Client, PeriodicClient]() { PeriodicClient->emitCachedValue(); });
    }

    Stream->getEpicsClient()->setServiceID(main_opt.MainSettings.ServiceID);
    for (auto &Converter : StreamInfo.Converters) {
      pushConverterToStream(Converter, Stream);
    }
  } catch (std::runtime_error &e) {
    std::throw_with_nested(MappingAddException("Cannot add stream"));
  }
}

template <typename T>
std::shared_ptr<Stream> Forwarder::findOrAddStream(
    ChannelInfo &ChannelInfo, std::shared_ptr<T> EpicsClient,
    std::shared_ptr<
        moodycamel::ConcurrentQueue<std::shared_ptr<FlatBufs::EpicsPVUpdate>>>
        UpdateQueue) {
  std::shared_ptr<Stream> FoundStream =
      streams.getStreamByChannelName(ChannelInfo.channel_name);
  if (FoundStream != nullptr) {
    return FoundStream;
  }
  auto EpicsClientInterfacePtr =
      std::static_pointer_cast<EpicsClient::EpicsClientInterface>(EpicsClient);
  auto NewStream = std::make_shared<Stream>(
      ChannelInfo, EpicsClientInterfacePtr, UpdateQueue);
  streams.add(NewStream);
  return NewStream;
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
