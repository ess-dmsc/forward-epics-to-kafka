#pragma once

#include "uri.h"
#include <atomic>
#include <deque>
#include <map>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace Forwarder {

/// Holder for the Kafka brokers settings defined in the configuration file.
struct KafkaBrokerSettings {
  std::map<std::string, int64_t> ConfigurationIntegers;
  std::map<std::string, std::string> ConfigurationStrings;
};

/// Holder for the converter settings defined in the configuration file.
struct ConverterSettings {
  std::string Schema;
  std::string Topic;
  std::string Name;
};

/// Holder for the stream settings defined in the configuration file.
struct StreamSettings {
  std::string Name;
  std::string EpicsProtocol;
  std::vector<ConverterSettings> Converters;
};

/// Holder for the configuration settings defined in the configuration file.
struct ConfigSettings {
  URI BrokerConfig{"//localhost:9092/forward_epics_to_kafka_commands"};
  std::vector<URI> Brokers;
  size_t ConversionThreads{1};
  size_t ConversionWorkerQueueSize{1024};
  int32_t MainPollInterval{500};
  URI StatusReportURI;
  KafkaBrokerSettings BrokerSettings;
  std::vector<StreamSettings> StreamsInfo;
  std::map<std::string, KafkaBrokerSettings> GlobalConverters;
};

/// Class responsible for parsing the JSON configuration information.
class ConfigParser {
public:
  /// Constructor.
  ConfigParser() = default;

  /// Set the JSON string to be parsed.
  ///
  /// \param RawJson The JSON to be parsed.
  void setJsonFromString(std::string RawJson);

  /// Extract the configuration information from the JSON.
  ///
  /// \return The extracted settings.
  ConfigSettings extractConfiguration();

  /// Set the broker(s) where the forwarded data will be written.
  ///
  /// \param Brokers The raw brokers information, e.g. "localhost:9092"
  /// \param Settings The settings to write the brokers to.
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
} // namespace Forwarder
