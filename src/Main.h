#pragma once
#include <list>
#include <map>
#include <algorithm>
#include <mutex>
#include <atomic>
#include "MainOpt.h"
#include "ForwarderInfo.h"
#include "ConversionWorker.h"

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

struct stub_curl;

class Main {
public:
  Main(MainOpt &opt);
  ~Main();
  void forward_epics_to_kafka();
  int mapping_add(rapidjson::Value &mapping);
  void forwarding_exit();
  void report_stats(int started_in_current_round);
  void stop();
  void check_stream_status();
  int streams_clear();
  int channel_stop(std::string const &channel);
  int conversion_workers_clear();
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
  std::map<std::string, std::weak_ptr<Converter> > converters;
  std::mutex streams_mutex;
  std::vector<std::unique_ptr<Stream> > streams;
  std::mutex conversion_workers_mx;
  std::vector<std::unique_ptr<ConversionWorker> > conversion_workers;
  ConversionScheduler conversion_scheduler;
  friend class ConfigCB;
  friend class tests::Remote_T;
  friend class ConversionScheduler;
  std::atomic<uint32_t> converter_ix{ 0 };
  std::atomic<int32_t> forwarding_run{ 1 };
  std::atomic<ForwardingStatus> forwarding_status{ ForwardingStatus::NORMAL };
  std::unique_ptr<stub_curl> curl;
};

extern std::atomic<uint64_t> g__total_msgs_to_kafka;
extern std::atomic<uint64_t> g__total_bytes_to_kafka;
}
}
