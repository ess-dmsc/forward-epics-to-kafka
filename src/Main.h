#pragma once

#include "Config.h"
#include "ConversionWorker.h"
#include "MainOpt.h"
#include "PeriodicPVPoller.h"
#include "Streams.h"
#include <algorithm>
#include <atomic>
#include <list>
#include <map>
#include <mutex>
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace BrightnESS {
namespace ForwardEpicsToKafka {

class MappingAddException : public std::runtime_error {
public:
  explicit MappingAddException(std::string what) : std::runtime_error(what) {}
};

class Converter;
class Stream;
namespace tests {
class Remote_T;
}

namespace Config {
class Listener;
}

struct CURLReporter;

enum class ForwardingRunState : int {
  RUN = 0,
  STOP = 1,
  STOP_DUE_TO_SIGNAL = 2,
  STOPPED = 3
};

class Main {
public:
  Main(MainOpt &opt);
  ~Main();
  void forward_epics_to_kafka();
  void mappingAdd(nlohmann::json const &Mapping);
  void stopForwarding();
  void stopForwardingDueToSignal();
  void report_status();
  void report_stats(int started_in_current_round);
  int conversion_workers_clear();
  int converters_clear();
  std::unique_lock<std::mutex> get_lock_streams();
  std::unique_lock<std::mutex> get_lock_converters();

  // Public for unit testing
  void extractConverterInfo(const nlohmann::json &JSON, std::string &Schema,
                            std::string &Topic, std::string &ConverterName);
  void extractMappingInfo(nlohmann::json const &Mapping, std::string &Channel,
                          std::string &ChannelProviderType);

private:
  MainOpt &main_opt;
  std::shared_ptr<Kafka::InstanceSet> kafka_instance_set;
  std::unique_ptr<PeriodicPVPoller> PVPoller;
  std::unique_ptr<Config::Listener> config_listener;
  std::mutex converters_mutex;
  std::map<std::string, std::weak_ptr<Converter>> converters;
  std::mutex streams_mutex;
  std::mutex conversion_workers_mx;
  std::vector<std::unique_ptr<ConversionWorker>> conversion_workers;
  ConversionScheduler conversion_scheduler;
  friend class ConfigCB;
  friend class tests::Remote_T;
  friend class ConversionScheduler;
  std::atomic<uint32_t> converter_index{0};
  std::unique_ptr<CURLReporter> curl;
  std::shared_ptr<KafkaW::Producer> status_producer;
  std::unique_ptr<KafkaW::ProducerTopic> status_producer_topic;
  Streams streams;
  std::atomic<ForwardingRunState> ForwardingRunFlag{ForwardingRunState::RUN};
  void raiseForwardingFlag(ForwardingRunState ToBeRaised);
  void
  pushConverterToStream(nlohmann::json const &JSON,
                        std::shared_ptr<ForwardEpicsToKafka::Stream> &Stream);
  template <typename T> std::shared_ptr<T> addStream(ChannelInfo &ChannelInfo);
};

/// \brief Helper class to provide a callback for the Kafka command listener.
class ConfigCB : public Config::Callback {
public:
  ConfigCB(Main &main);
  // This is called from the same thread as the main watchdog below, because the
  // code below calls the config poll which in turn calls this callback.
  void operator()(std::string const &msg) override;
  void handleCommand(std::string const &Msg);
  void handleCommandAdd(nlohmann::json const &Document);
  void handleCommandStopChannel(nlohmann::json const &Document);
  void handleCommandStopAll();
  void handleCommandExit();
  std::string findCommand(nlohmann::json const &Document);

private:
  Main &main;
};

extern std::atomic<uint64_t> g__total_msgs_to_kafka;
extern std::atomic<uint64_t> g__total_bytes_to_kafka;
} // namespace ForwardEpicsToKafka
} // namespace BrightnESS
