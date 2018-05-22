#pragma once

#include "uri.h"
#include <atomic>
#include <deque>
#include <map>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

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
  KafkaBrokerSettings BrokerSettings;
  std::vector<StreamSettings> StreamsInfo;
  std::map<std::string, KafkaBrokerSettings> GlobalConverters;
};

class ConfigParser {
public:
  ConfigParser() = default;
  void setJsonFromString(std::string RawJson);
  ConfigSettings extractConfiguration();
  static void setBrokers(std::string const &Brokers, ConfigSettings &Settings);

private:
  nlohmann::json Json;
  void extractMappingInfo(nlohmann::json const &Mapping, std::string &Channel,
                          std::string &Protocol);
  ConverterSettings extractConverterSettings(nlohmann::json const &Mapping);
  void extractBrokerConfig(ConfigSettings &Settings);
  void extractBrokers(ConfigSettings &Settings);
  void extractConversionThreads(ConfigSettings &Settings);
  void extractConversionWorkerQueueSize(ConfigSettings &Settings);
  void extractMainPollInterval(ConfigSettings &Settings);
  void extractStatusUri(ConfigSettings &Settings);
  void extractKafkaBrokerSettings(ConfigSettings &Settings);
  void extractStreamSettings(ConfigSettings &Settings);
  void extractGlobalConverters(ConfigSettings &Settings);
  std::atomic<uint32_t> ConverterIndex{0};
};
} // namespace ForwardEpicsToKafka
} // namespace BrightnESS
