#pragma once

#include "URI.h"
#include <atomic>
#include <deque>
#include <map>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace Forwarder {

/// Holder for the converter settings defined in the streams configuration file.
struct ConverterSettings {
  std::string Schema;
  std::string Topic;
  std::string Name;
};

/// Holder for the stream settings defined in the streams configuration file.
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
  std::string ServiceID;
  URI StatusReportURI;
  std::map<std::string, std::string> KafkaConfiguration;
  std::vector<StreamSettings> StreamsInfo;
  std::map<std::string, std::map<std::string, std::string>> GlobalConverters;
};

/// Class responsible for parsing the JSON configuration information.
class ConfigParser {
public:
  /// Constructor
  ///
  /// \param RawJson The JSON to be parsed.
  explicit ConfigParser(const std::string &RawJson);

  /// Extract the configuration information from the JSON.
  ///
  /// \return The extracted settings.
  ConfigSettings extractStreamInfo();

  /// Set the broker(s) where the forwarded data will be written.
  ///
  /// \param Brokers The raw brokers information, e.g. "localhost:9092"
  /// \param Settings The settings to write the brokers to.
  static void setBrokers(std::string const &Brokers, ConfigSettings &Settings);

private:
  nlohmann::json Json;
  static void extractMappingInfo(nlohmann::json const &Mapping,
                                 std::string &Channel, std::string &Protocol);
  ConverterSettings extractConverterSettings(nlohmann::json const &Mapping);
  std::atomic<uint32_t> ConverterIndex{0};
};
} // namespace Forwarder
