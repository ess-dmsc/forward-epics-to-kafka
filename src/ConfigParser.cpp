#include "ConfigParser.h"
#include "helper.h"
#include "json.h"
#include "logger.h"
#include "Main.h"
#include <iostream>

namespace BrightnESS {
namespace ForwardEpicsToKafka {

void ConfigParser::setJsonFromString(std::string RawJson) {
  Json = nlohmann::json::parse(RawJson);

  if (Json.is_null()) {
    throw std::runtime_error("Cannot parse configuration file as JSON");
  }
}

void ConfigParser::extractConfiguration() {
  extractBrokerConfig();
  extractBrokers();
  extractConversionThreads();
  extractConversionWorkerQueueSize();
  extractMainPollInterval();
  extractStatusUri();
  extractKafkaBrokerSettings();
  extractStreamSettings();
}

void ConfigParser::extractBrokerConfig() {
  if (auto x = find<std::string>("broker-config", Json)) {
    Settings.BrokerConfig = x.inner();
  }
}

void ConfigParser::extractBrokers() {
  if (auto x = find<std::string>("broker", Json)) {
    setBrokers(x.inner());
  } else {
    // If not specified then use default
    setBrokers("localhost:9092");
  }
}

void ConfigParser::extractConversionThreads() {
  if (auto x = find<size_t>("conversion-threads", Json)) {
    Settings.ConversionThreads = x.inner();
  }
}

void ConfigParser::extractConversionWorkerQueueSize() {
  if (auto x =
      find<size_t>("conversion-worker-queue-size", Json)) {
    Settings.ConversionWorkerQueueSize = x.inner();
  }
}

void ConfigParser::extractMainPollInterval() {
  if (auto x = find<int32_t>("main-poll-interval", Json)) {
    Settings.MainPollInterval = x.inner();
  }
}

void ConfigParser::extractGlobalConverters(std::string &Schema) {
  using nlohmann::json;

  if (auto ConvertersMaybe = find<json>("converters", Json)) {
    auto const &Converters = ConvertersMaybe.inner();
    if (Converters.is_object()) {
      if (auto SchemaMaybe = find<json>(Schema, Converters)) {
        auto const &ConverterSchemaConfig = SchemaMaybe.inner();
        if (ConverterSchemaConfig.is_object()) {
          for (auto SettingIt = ConverterSchemaConfig.begin();
               SettingIt != ConverterSchemaConfig.end(); ++SettingIt) {
            if (SettingIt.value().is_number()) {
              Settings.ConverterInts[SettingIt.key()] = SettingIt.value().get<int64_t>();
            }

            if (SettingIt.value().is_string()) {
              Settings.ConverterStrings[SettingIt.key()] =
                  SettingIt.value().get<std::string>();
            }
          }
        }
      }
    }
  }
}

void ConfigParser::extractStatusUri() {
  if (auto x = find<std::string>("status-uri", Json)) {
    auto URIString = x.inner();
    uri::URI URI;
    URI.port = 9092;
    URI.parse(URIString);
    Settings.StatusReportURI = URI;
  }
}

void ConfigParser::setBrokers(std::string Broker) {
  Settings.Brokers.clear();
  auto a = split(Broker, ",");
  for (auto &x : a) {
    uri::URI u1;
    u1.require_host_slashes = false;
    u1.parse(x);
    Settings.Brokers.push_back(u1);
  }
}

void ConfigParser::extractKafkaBrokerSettings() {
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

void ConfigParser::extractStreamSettings() {
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
              Stream.Converters.push_back(extractConverterSettings(ConverterSettings));
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

  auto ChannelMaybe = find<std::string>("channel", Mapping);
  if (!ChannelMaybe) {
    throw MappingAddException("Can not find channel");
  }
  Channel = ChannelMaybe.inner();

  auto ChannelProviderTypeMaybe =
      find<std::string>("channel_provider_type", Mapping);
  if (ChannelProviderTypeMaybe) {
    Protocol = ChannelProviderTypeMaybe.inner();
  } else {
    // Default is pva
    Protocol = "pva";
  }

}

ConverterSettings ConfigParser::extractConverterSettings(nlohmann::json const &Mapping) {
  ConverterSettings Settings;
  Settings.Schema = find<std::string>("schema", Mapping).inner();
  Settings.Topic = find<std::string>("topic", Mapping).inner();
  if (auto x = find<std::string>("name", Mapping)) {
    Settings.Name = x.inner();
  } else {
    // Assign automatically generated name
    Settings.Name = fmt::format("converter_{}", ConverterIndex++);
  }

  return Settings;
}

}
}


