#pragma once

#include "Config.h"
#include "ConversionWorker.h"
#include "ForwarderInfo.h"
#include "MainOpt.h"
#include "Streams.h"
#include <algorithm>
#include <atomic>
#include <list>
#include <map>
#include <mutex>
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

enum class ForwardingStatus : int32_t {
  NORMAL,
  STOPPED,
};

class CURLReporter;

enum class ForwardingRunState : int {
  RUN = 0,
  STOP = 1,
  STOP_DUE_TO_SIGNAL = 2,
};

class Main {
public:
  explicit Main(MainOpt &opt);
  ~Main();
  void forward_epics_to_kafka();
  void addMapping(StreamSettings const &Stream);
  void stopForwarding();
  void stopForwardingDueToSignal();
  void report_status();
  void report_stats(int started_in_current_round);
  int conversion_workers_clear();
  int converters_clear();
  std::unique_lock<std::mutex> get_lock_streams();
  std::unique_lock<std::mutex> get_lock_converters();

private:
  MainOpt &main_opt;
  std::shared_ptr<ForwarderInfo> finfo;
  std::shared_ptr<Kafka::InstanceSet> kafka_instance_set;
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
  std::atomic<ForwardingStatus> forwarding_status{ForwardingStatus::NORMAL};
  std::unique_ptr<CURLReporter> curl;
  std::shared_ptr<KafkaW::Producer> status_producer;
  std::unique_ptr<KafkaW::ProducerTopic> status_producer_topic;
  Streams streams;
  std::atomic<ForwardingRunState> ForwardingRunFlag{ForwardingRunState::RUN};
  void raiseForwardingFlag(ForwardingRunState ToBeRaised);
  void pushConverterToStream(
      ConverterSettings const &Converter,
      std::shared_ptr<ForwardEpicsToKafka::Stream> &Stream);
};

extern std::atomic<uint64_t> g__total_msgs_to_kafka;
extern std::atomic<uint64_t> g__total_bytes_to_kafka;
} // namespace ForwardEpicsToKafka
} // namespace BrightnESS
