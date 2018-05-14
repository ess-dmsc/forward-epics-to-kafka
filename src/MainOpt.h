#pragma once

#include "KafkaW/KafkaW.h"
#include "SchemaRegistry.h"
#include "ConfigParser.h"
#include "uri.h"
#include <string>
#include <vector>
#include <memory>

namespace BrightnESS {
namespace ForwardEpicsToKafka {

struct MainOpt {
  MainOpt();
  void set_broker(std::string& Broker);
  std::string brokers_as_comma_list() const;
  uri::URI BrokerConfig{"//localhost:9092/forward_epics_to_kafka_commands"};
  uri::URI StatusReportURI;
  std::vector<uri::URI> brokers;
  std::string KafkaGELFAddress = "";
  std::string GraylogLoggerAddress = "";
  std::string InfluxURI = "";
  size_t ConversionThreads = 1;
  size_t ConversionWorkerQueueSize = 1024;
  std::string LogFilename;
  std::string ConfigurationFile;
  int main_poll_interval = 500;
  uint64_t teamid = 0;
  std::vector<char> Hostname;
  FlatBufs::SchemaRegistry schema_registry;
  void parse_json_file(std::string ConfigurationFile);
  KafkaW::BrokerSettings broker_opt;
  void init_logger();
  void parse_document(const std::string &filepath);

  std::unique_ptr<ConfigParser> Config;
};

std::pair<int, std::unique_ptr<MainOpt>> parse_opt(int argc, char **argv);

}
}
