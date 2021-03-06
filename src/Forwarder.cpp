// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

#ifdef _MSC_VER
#include <WinSock2.h>
#include <windows.h>
#endif
#include "CommandHandler.h"
#include "Converter.h"
#include "EpicsClient/EpicsClientInterface.h"
#include "EpicsClient/EpicsClientMonitor.h"
#include "EpicsClient/EpicsClientRandom.h"
#include "Forwarder.h"
#include "KafkaOutput.h"
#include "MetricsReporter.h"
#include "StatusReporter.h"
#include "Stream.h"
#include "Timer.h"
#include "logger.h"
#include "schemas/f142/f142.cpp"
#include "schemas/tdc_time/TdcTime.h"
#include <algorithm>
#include <memory>
#include <nlohmann/json.hpp>
#include <sys/types.h>

namespace {
void registerSchemas() {
  FlatBufs::SchemaRegistry::Registrar<FlatBufs::SchemaInfo> Reg1(
      "f142", FlatBufs::SchemaInfo::ptr(new FlatBufs::f142::Info));
  FlatBufs::SchemaRegistry::Registrar<FlatBufs::SchemaInfo> Reg2(
      "TdcTime", TdcTime::Info::ptr(new TdcTime::Info));
}
} // namespace

namespace Forwarder {

static bool isStopDueToSignal(ForwardingRunState Flag) {
  return static_cast<int>(Flag) &
         static_cast<int>(ForwardingRunState::STOP_DUE_TO_SIGNAL);
}

/// Main program entry class.
Forwarder::Forwarder(MainOpt &Opt)
    : main_opt(Opt),
      KafkaInstanceSet(std::make_unique<InstanceSet>(Opt.GlobalBrokerSettings)),
      conversion_scheduler(this) {

  registerSchemas();

  for (size_t i = 0; i < Opt.MainSettings.ConversionThreads; ++i) {
    conversion_workers.emplace_back(std::make_unique<ConversionWorker>(
        &conversion_scheduler,
        static_cast<uint32_t>(Opt.MainSettings.ConversionWorkerQueueSize)));
  }

  createConfigListener();
  createPVUpdateTimerIfRequired();
  createFakePVUpdateTimerIfRequired();

  for (auto &Stream : main_opt.MainSettings.StreamsInfo) {
    try {
      addMapping(Stream);
    } catch (std::exception &e) {
      Logger->warn("Could not add mapping: {}  {}", Stream.Name, e.what());
    }
  }

  if (!main_opt.MainSettings.StatusReportURI.HostPort.empty()) {
    KafkaW::BrokerSettings BrokerSettings;
    BrokerSettings.Address = main_opt.MainSettings.StatusReportURI.HostPort;
    status_producer = std::make_shared<KafkaW::Producer>(BrokerSettings);
  }
}

void Forwarder::createConfigListener() {
  KafkaW::BrokerSettings ConsumerSettings;
  ConsumerSettings.Address = main_opt.MainSettings.BrokerConfig.HostPort;
  ConsumerSettings.PollTimeoutMS = 0;
  auto NewConsumer = std::make_unique<KafkaW::Consumer>(ConsumerSettings);
  config_listener = std::make_unique<Config::Listener>(
      main_opt.MainSettings.BrokerConfig, std::move(NewConsumer));
}

Forwarder::~Forwarder() {
  Logger->trace("~Main");
  streams.clearStreams();
  conversion_workers_clear();
  converters_clear();
}

void Forwarder::createPVUpdateTimerIfRequired() {
  if (main_opt.PeriodMS > 0) {
    auto Interval = std::chrono::milliseconds(main_opt.PeriodMS);
    PVUpdateTimer = std::make_unique<Timer>(Interval);
  }
}

void Forwarder::createFakePVUpdateTimerIfRequired() {
  if (main_opt.FakePVPeriodMS > 0) {
    auto Interval = std::chrono::milliseconds(main_opt.FakePVPeriodMS);
    GenerateFakePVUpdateTimer = std::make_unique<Timer>(Interval);
  }
}

int Forwarder::conversion_workers_clear() {
  Logger->trace("Main::conversion_workers_clear()  begin");
  std::lock_guard<std::mutex> lock(conversion_workers_mx);
  if (!conversion_workers.empty()) {
    for (auto &x : conversion_workers) {
      x->stop();
    }
    conversion_workers.clear();
  }
  Logger->trace("Main::conversion_workers_clear()  end");
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
  using namespace std::chrono_literals;
  ConfigCallback config_cb(*this);
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

  using namespace std::chrono_literals;
  MetricsReporter MetricsTimerInstance(2000ms, main_opt, KafkaInstanceSet);
  std::unique_ptr<KafkaW::ProducerTopic> status_producer_topic;
  if (!main_opt.MainSettings.StatusReportURI.HostPort.empty()) {
    status_producer_topic = std::make_unique<KafkaW::ProducerTopic>(
        status_producer, main_opt.MainSettings.StatusReportURI.Topic);
  }
  StatusReporter StatusTimerInstance(4000ms, main_opt, status_producer_topic,
                                     streams);

  while (ForwardingRunFlag.load() == ForwardingRunState::RUN) {
    if (config_listener) {
      config_listener->poll(config_cb);
    }
    KafkaInstanceSet->poll();
  }
  if (isStopDueToSignal(ForwardingRunFlag.load())) {
    Logger->info("Forwarder stopping due to signal.");
  }
  Logger->info("Main::forward_epics_to_kafka shutting down");
  conversion_workers_clear();
  streams.clearStreams();

  if (PVUpdateTimer != nullptr) {
    PVUpdateTimer->waitForStop();
  }

  if (GenerateFakePVUpdateTimer != nullptr) {
    GenerateFakePVUpdateTimer->waitForStop();
  }

  Logger->info("ForwardingStatus::STOPPED");
  forwarding_status.store(ForwardingStatus::STOPPED);
}

URI Forwarder::createTopicURI(ConverterSettings const &ConverterInfo) const {
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
  } catch (std::runtime_error &) {
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
  auto Topic = KafkaInstanceSet->createProducerTopic(std::move(TopicURI));
  auto cp = std::make_unique<ConversionPath>(
      std::move(ConverterShared),
      std::make_unique<KafkaOutput>(std::move(Topic)));

  Stream->addConverter(std::move(cp));
}

void Forwarder::addMapping(StreamSettings const &StreamInfo) {
  auto lock = get_lock_streams();
  try {
    ChannelInfo ChannelInfo{StreamInfo.EpicsProtocol, StreamInfo.Name};
    std::shared_ptr<Stream> Stream;
    if (GenerateFakePVUpdateTimer != nullptr) {
      Stream = addStream<EpicsClient::EpicsClientRandom>(ChannelInfo);
      auto Client = Stream->getEpicsClient();
      auto RandomClient =
          dynamic_cast<EpicsClient::EpicsClientRandom *>(Client.get());
      if (RandomClient) {
        GenerateFakePVUpdateTimer->addCallback(
            [Client, RandomClient]() { RandomClient->generateFakePVUpdate(); });
      }
    } else {
      Stream = addStream<EpicsClient::EpicsClientMonitor>(ChannelInfo);
    }

    if (PVUpdateTimer != nullptr) {
      auto Client = Stream->getEpicsClient();
      auto PeriodicClient =
          dynamic_cast<EpicsClient::EpicsClientMonitor *>(Client.get());
      PVUpdateTimer->addCallback(
          [Client, PeriodicClient]() { PeriodicClient->emitCachedValue(); });
    }
    if (StreamInfo.Converters.size() > 0) {
      auto ConnectionStatusProducer = KafkaInstanceSet->createProducerTopic(
          URI(StreamInfo.Converters[0].Topic));
      Stream->getEpicsClient()->setProducer(
          std::make_unique<KafkaW::ProducerTopic>(
              std::move(ConnectionStatusProducer)));
    }
    Stream->getEpicsClient()->setServiceID(main_opt.MainSettings.ServiceID);
    for (auto &Converter : StreamInfo.Converters) {

      pushConverterToStream(Converter, Stream);
    }
  } catch (std::runtime_error &) {
    std::throw_with_nested(MappingAddException("Cannot add stream"));
  }
}

template <typename T>
std::shared_ptr<Stream> Forwarder::addStream(ChannelInfo &ChannelInfo) {
  std::shared_ptr<Stream> FoundStream =
      streams.getStreamByChannelName(ChannelInfo.channel_name);
  if (FoundStream != nullptr) {
    Logger->warn("Could not add stream for {} as one already exists.",
                 ChannelInfo.channel_name);
    throw MappingAddException("Stream already exists");
  }
  auto PVUpdateRing = std::make_shared<
      moodycamel::ConcurrentQueue<std::shared_ptr<FlatBufs::EpicsPVUpdate>>>();
  auto Client = std::make_shared<T>(ChannelInfo, PVUpdateRing);
  auto EpicsClientInterfacePtr =
      std::static_pointer_cast<EpicsClient::EpicsClientInterface>(Client);
  auto NewStream = std::make_shared<Stream>(
      ChannelInfo, EpicsClientInterfacePtr, PVUpdateRing);
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
