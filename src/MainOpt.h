#pragma once

#include "KafkaW/KafkaW.h"
#include "SchemaRegistry.h"
#include "uri.h"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace BrightnESS {
namespace ForwardEpicsToKafka {

struct MainOpt {
  MainOpt();
  void set_broker(std::string broker);
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
  int periodMS = 0;
  std::vector<char> Hostname;
  FlatBufs::SchemaRegistry schema_registry;
  nlohmann::json JSONConfiguration;
  void parse_json_file(std::string ConfigurationFile);
  KafkaW::BrokerSettings broker_opt;
  void init_logger();
  void find_status_uri();
  void parse_document(const std::string &filepath);
  void findBroker();
  void findBrokerConfig();
  void findConversionThreads();
  void findConversionWorkerQueueSize();
  void findMainPollInterval();
};

std::pair<int, std::unique_ptr<MainOpt>> parse_opt(int argc, char **argv);

struct KafkaBrokerSettings {
  std::map<std::string, int64_t> ConfigurationIntegers;
  std::map<std::string, std::string> ConfigurationStrings;
};

KafkaBrokerSettings
extractKafkaBrokerSettingsFromJSON(nlohmann::json const &JSONConfiguration);
}
}
