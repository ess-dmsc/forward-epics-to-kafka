#include "ConfigParser.h"
#include "Forwarder.h"
#include "helper.h"
#include "json.h"
#include "logger.h"
#include <iostream>

namespace Forwarder {

void ConfigParser::setJsonFromString(std::string RawJson) {
  Json = nlohmann::json::parse(RawJson);

  if (Json.is_null()) {
    throw std::runtime_error("Cannot parse configuration file as JSON");
  }
}

ConfigSettings ConfigParser::extractConfiguration() {
  ConfigSettings Settings;

  extractBrokerConfig(Settings);
  extractBrokers(Settings);
  extractConversionThreads(Settings);
  extractConversionWorkerQueueSize(Settings);
  extractMainPollInterval(Settings);
  extractStatusUri(Settings);
  extractKafkaBrokerSettings(Settings);
  extractStreamSettings(Settings);
  extractGlobalConverters(Settings);

  return Settings;
}

void ConfigParser::extractBrokerConfig(ConfigSettings &Settings) {
  if (auto x = find<std::string>("broker-config", Json)) {
    Settings.BrokerConfig = x.inner();
  }
}

void ConfigParser::extractBrokers(ConfigSettings &Settings) {
  if (auto x = find<std::string>("broker", Json)) {
    setBrokers(x.inner(), Settings);
  } else {
    // If not specified then use default
    setBrokers("localhost:9092", Settings);
  }
}

void ConfigParser::extractConversionThreads(ConfigSettings &Settings) {
  if (auto x = find<size_t>("conversion-threads", Json)) {
    Settings.ConversionThreads = x.inner();
  }
}

void ConfigParser::extractConversionWorkerQueueSize(ConfigSettings &Settings) {
  if (auto x = find<size_t>("conversion-worker-queue-size", Json)) {
    Settings.ConversionWorkerQueueSize = x.inner();
  }
}

void ConfigParser::extractMainPollInterval(ConfigSettings &Settings) {
  if (auto x = find<int32_t>("main-poll-interval", Json)) {
    Settings.MainPollInterval = x.inner();
  }
}

void ConfigParser::extractGlobalConverters(ConfigSettings &Settings) {
  using nlohmann::json;

  if (auto ConvertersMaybe = find<json>("converters", Json)) {
    auto const &Converters = ConvertersMaybe.inner();
    if (Converters.is_object()) {
      for (auto It = Converters.begin(); It != Converters.end(); ++It) {
        KafkaBrokerSettings BrokerSettings{};
        for (auto SettingIt = It.value().begin(); SettingIt != It.value().end();
             ++SettingIt) {
          if (SettingIt.value().is_number()) {
            BrokerSettings.ConfigurationIntegers[SettingIt.key()] =
                SettingIt.value().get<int64_t>();
          }

          if (SettingIt.value().is_string()) {
            BrokerSettings.ConfigurationStrings[SettingIt.key()] =
                SettingIt.value().get<std::string>();
          }
        }
        Settings.GlobalConverters[It.key()] = BrokerSettings;
      }
    }
  }
}

void ConfigParser::extractStatusUri(ConfigSettings &Settings) {
  if (auto x = find<std::string>("status-uri", Json)) {
    auto URIString = x.inner();
    URI URI;
    URI.port = 9092;
    URI.parse(URIString);
    Settings.StatusReportURI = URI;
  }
}

void ConfigParser::setBrokers(std::string const &Brokers,
                              ConfigSettings &Settings) {
  Settings.Brokers.clear();
  auto a = split(Brokers, ",");
  for (auto &x : a) {
    URI u1;
    u1.require_host_slashes = false;
    u1.parse(x);
    Settings.Brokers.push_back(u1);
  }
}

void ConfigParser::extractKafkaBrokerSettings(ConfigSettings &Settings) {
  using nlohmann::json;

  if (auto KafkaMaybe = find<json>("kafka", Json)) {
    auto Kafka = KafkaMaybe.inner();
    if (auto BrokerMaybe = find<json>("broker", Kafka)) {
      auto Broker = BrokerMaybe.inner();
      for (auto Property = Broker.begin(); Property != Broker.end();
           ++Property) {
        auto const Key = Property.key();
        if (Key.find("___") == 0) {
          // Skip this property
          continue;
        }
        if (Property.value().is_string()) {
          auto Value = Property.value().get<std::string>();
          LOG(6, "kafka broker config {}: {}", Key, Value);
          Settings.BrokerSettings.ConfigurationStrings[Key] = Value;
        } else if (Property.value().is_number()) {
          auto Value = Property.value().get<int64_t>();
          LOG(6, "kafka broker config {}: {}", Key, Value);
          Settings.BrokerSettings.ConfigurationIntegers[Key] = Value;
        } else {
          LOG(3, "can not understand option: {}", Key);
        }
      }
    }
  }
}

void ConfigParser::extractStreamSettings(ConfigSettings &Settings) {
  using nlohmann::json;
  if (auto StreamsMaybe = find<json>("streams", Json)) {
    auto Streams = StreamsMaybe.inner();
    if (Streams.is_array()) {
      for (auto const &StreamJson : Streams) {
        StreamSettings Stream;
        // Find the basic information
        extractMappingInfo(StreamJson, Stream.Name, Stream.EpicsProtocol);

        // Find the converters, if present
        if (auto x = find<nlohmann::json>("converter", StreamJson)) {
          if (x.inner().is_object()) {
            Stream.Converters.push_back(extractConverterSettings(x.inner()));
          } else if (x.inner().is_array()) {
            for (auto const &ConverterSettings : x.inner()) {
              Stream.Converters.push_back(
                  extractConverterSettings(ConverterSettings));
            }
          }
        }

        Settings.StreamsInfo.push_back(Stream);
      }
    }
  }
}

void ConfigParser::extractMappingInfo(nlohmann::json const &Mapping,
                                      std::string &Channel,
                                      std::string &Protocol) {
  if (!Mapping.is_object()) {
    throw MappingAddException("Given Mapping is not a JSON object");
  }

  if (auto ChannelMaybe = find<std::string>("channel", Mapping)) {
    Channel = ChannelMaybe.inner();
  } else {
    throw MappingAddException("Cannot find channel");
  }

  if (auto ChannelProviderTypeMaybe =
          find<std::string>("channel_provider_type", Mapping)) {
    Protocol = ChannelProviderTypeMaybe.inner();
  } else {
    // Default is pva
    Protocol = "pva";
  }
}

ConverterSettings
ConfigParser::extractConverterSettings(nlohmann::json const &Mapping) {
  ConverterSettings Settings;

  if (auto SchemaMaybe = find<std::string>("schema", Mapping)) {
    Settings.Schema = SchemaMaybe.inner();
  } else {
    throw MappingAddException("Cannot find schema");
  }

  if (auto TopicMaybe = find<std::string>("topic", Mapping)) {
    Settings.Topic = TopicMaybe.inner();
  } else {
    throw MappingAddException("Cannot find topic");
  }

  if (auto x = find<std::string>("name", Mapping)) {
    Settings.Name = x.inner();
  } else {
    // Assign automatically generated name
    Settings.Name = fmt::format("converter_{}", ConverterIndex++);
  }

  return Settings;
}

} // namespace Forwarder
