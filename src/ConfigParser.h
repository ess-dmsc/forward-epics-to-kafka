#pragma once

#include <deque>
#include <string>
#include <vector>
#include <map>
#include <atomic>
#include <nlohmann/json.hpp>
#include "uri.h"

namespace BrightnESS {
namespace ForwardEpicsToKafka {

struct KafkaBrokerSettings {
  std::map<std::string, int64_t> ConfigurationIntegers;
  std::map<std::string, std::string> ConfigurationStrings;
};

struct ConverterSettings {
  std::string Schema;
  std::string Topic;
  std::string Name;
};

struct StreamSettings {
  std::string Name;
  std::string EpicsProtocol;
  std::vector<ConverterSettings> Converters;
};

struct ConfigSettings {
  uri::URI BrokerConfig{"//localhost:9092/forward_epics_to_kafka_commands"};
  std::vector<uri::URI> Brokers;
  size_t ConversionThreads{1};
  size_t ConversionWorkerQueueSize{1024};
  int32_t MainPollInterval{500};
  uri::URI StatusReportURI;
  std::map<std::string, int64_t> ConverterInts;
  std::map<std::string, std::string> ConverterStrings;
  KafkaBrokerSettings BrokerSettings;
  std::vector<StreamSettings> StreamsInfo;
};

class ConfigParser {
public:
  ConfigParser() = default;
  void setJsonFromString(std::string RawJson);
  void extractConfiguration();
  void extractGlobalConverters(std::string &Schema);
  void setBrokers(std::string Brokers);
  ConfigSettings Settings;

private:
  nlohmann::json Json;
  void extractMappingInfo(nlohmann::json const &Mapping,
                          std::string &Channel,
                          std::string &Protocol);
  ConverterSettings extractConverterSettings(nlohmann::json const &Mapping);
  void extractBrokerConfig();
  void extractBrokers();
  void extractConversionThreads();
  void extractConversionWorkerQueueSize();
  void extractMainPollInterval();
  void extractStatusUri();
  void extractKafkaBrokerSettings();
  void extractStreamSettings();
  std::atomic<uint32_t> ConverterIndex{0};
};
}
}
