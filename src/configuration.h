#pragma once

#include <deque>
#include <string>
#include <vector>
#include <map>
#include <nlohmann/json.hpp>
#include "uri.h"

namespace BrightnESS {
namespace ForwardEpicsToKafka {

struct KafkaBrokerSettings {
  std::map<std::string, int64_t> ConfigurationIntegers;
  std::map<std::string, std::string> ConfigurationStrings;
};

class Configuration {
public:
  Configuration() = default;
  void setJsonFromString(std::string RawJson);
  void extractBrokerConfig();
  void extractBrokers();
  void extractConversionThreads();
  void extractConversionWorkerQueueSize();
  void extractMainPollInterval();
  void extractStatusUri();
  void extractGlobalConverters(std::string &Schema);
  void extractKafkaBrokerSettings();
  void setBrokers(std::string Brokers);
  uri::URI BrokerConfig{"//localhost:9092/forward_epics_to_kafka_commands"};
  std::vector<uri::URI> Brokers;
  size_t ConversionThreads;
  size_t ConversionWorkerQueueSize;
  int32_t MainPollInterval;
  uri::URI StatusReportURI;
  std::map<std::string, int64_t> ConverterInts;
  std::map<std::string, std::string> ConverterStrings;
  KafkaBrokerSettings BrokerSettings;

private:
  nlohmann::json Json;
};
}
}
