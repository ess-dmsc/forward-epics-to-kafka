#pragma once
#include "KafkaW.h"
#include "SchemaRegistry.h"
#include "uri.h"
#include <rapidjson/document.h>
#include <string>
#include <vector>

class MainOpt_T;

namespace BrightnESS {
namespace ForwardEpicsToKafka {

using std::string;
using std::vector;

struct MainOpt {
  MainOpt();
  void set_broker(string broker);
  std::string brokers_as_comma_list() const;
  uri::URI broker_config{"//localhost:9092/forward_epics_to_kafka_commands"};
  uri::URI status_uri;
  vector<uri::URI> brokers;
  string kafka_gelf = "";
  string graylog_logger_address = "";
  string influx_url = "";
  int conversion_threads = 1;
  uint32_t conversion_worker_queue_size = 1024;
  bool help = false;
  string log_file;
  string config_file;
  int forwarder_ix = 0;
  int write_per_message = 0;
  int main_poll_interval = 500;
  uint64_t teamid = 0;
  std::vector<char> hostname;
  FlatBufs::SchemaRegistry schema_registry;
  std::shared_ptr<rapidjson::Document> json;
  int parse_json_file(string config_file);
  KafkaW::BrokerOpt broker_opt;
  void init_logger();
  friend class ::MainOpt_T;
};

std::pair<int, std::unique_ptr<MainOpt>> parse_opt(int argc, char **argv);
}
}
