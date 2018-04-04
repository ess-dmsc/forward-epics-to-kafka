#pragma once
#include "KafkaW/KafkaW.h"
#include "SchemaRegistry.h"
#include "uri.h"
#ifdef _MSC_VER
#include "wingetopt.h"
#include <iso646.h>
#else
#include <getopt.h>
#endif
#include <rapidjson/document.h>
#include <string>
#include <vector>

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
  KafkaW::BrokerSettings broker_opt;
  void init_logger();
  void find_broker();
  void find_broker_config(uri::URI &property);
  void find_conversion_threads(int &property);
  void find_conversion_worker_queue_size(uint32_t &property);
  void find_main_poll_interval(int &property);
  void find_brokers_config();
  void find_status_uri();
  void parse_document(const std::string &filepath);
  void find_int(const char *key, int &property) const;
  void find_uint32_t(const char *key, uint32_t &property);
};

static string MAN_PAGE =
    ("Forwards EPICS process variables to Kafka topics.\n"
     "\n"
     "forward-epics-to-kafka\n"
     "  --help, -h\n"
     "\n"
     "  --config-file                     filename\n"
     "      Configuration file in JSON format.\n"
     "      To overwrite the options in config-file, specify them later on "
     "the command line.\n"
     "\n"
     "  --broker-config                   //host[:port]/topic\n"
     "      Kafka brokers to connect with for configuration updates.\n"
     "\n"
     "  --broker                          host:port,host:port,...\n"
     "      Kafka brokers to connect with for configuration updates\n"
     "      Default: {}\n"
     "\n"
     "  --kafka-gelf                      kafka://host[:port]/topic\n"
     "\n"
     "  --graylog-logger-address          host:port\n"
     "      Log to Graylog via graylog_logger library.\n"
     "\n"
     "  -v\n"
     "      Decrease log_level by one step.  Default log_level is 3.\n"
     "  -Q\n"
     "      Increase log_level by one step.\n"
     "\n");

void parse_long_argument(const char *lname,
                         std::pair<int, std::unique_ptr<MainOpt>> &ret,
                         MainOpt &opt);
std::pair<int, std::unique_ptr<MainOpt>> parse_opt(int argc, char **argv);
}
}
