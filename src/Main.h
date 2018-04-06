#pragma once
#include "ConversionWorker.h"
#include "ForwarderInfo.h"
#include "MainOpt.h"
#include "Streams.h"
#include <algorithm>
#include <atomic>
#include <list>
#include <map>
#include <mutex>

namespace BrightnESS {
namespace ForwardEpicsToKafka {

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

struct CURLReporter;

enum class ForwardingRunState : int {
  RUN = 0,
  STOP = 1,
  STOP_DUE_TO_SIGNAL = 2,
};

class Main {
public:
  Main(MainOpt &opt);
  ~Main();
  void forward_epics_to_kafka();
  int mapping_add(rapidjson::Value &mapping);
  void stopForwarding();
  void stopForwardingDueToSignal();
  void report_status();
  void report_stats(int started_in_current_round);
  int conversion_workers_clear();
  int converters_clear();
  std::unique_lock<std::mutex> get_lock_streams();
  std::unique_lock<std::mutex> get_lock_converters();

private:
  int const init_pool_max = 64;
  int const memory_release_grace_time = 45;
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
  std::atomic<uint32_t> converter_ix{0};
  std::atomic<ForwardingStatus> forwarding_status{ForwardingStatus::NORMAL};
  std::unique_ptr<CURLReporter> curl;
  std::shared_ptr<KafkaW::Producer> status_producer;
  std::unique_ptr<KafkaW::ProducerTopic> status_producer_topic;
  Streams streams;
  std::atomic<ForwardingRunState> ForwardingRunFlag{ForwardingRunState::RUN};
  void raiseForwardingFlag(ForwardingRunState ToBeRaised);
};

extern std::atomic<uint64_t> g__total_msgs_to_kafka;
extern std::atomic<uint64_t> g__total_bytes_to_kafka;
}
}
