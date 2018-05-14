#include "configuration.h"
#include "helper.h"
#include "json.h"
#include "logger.h"
#include "Main.h"
#include <iostream>

namespace BrightnESS {
namespace ForwardEpicsToKafka {

void Configuration::setJsonFromString(std::string RawJson) {
  Json = nlohmann::json::parse(RawJson);
}

void Configuration::extractBrokerConfig() {
  if (auto x = find<std::string>("broker-config", Json)) {
    BrokerConfig = x.inner();
  }
}

void Configuration::extractBrokers() {
  if (auto x = find<std::string>("broker", Json)) {
    setBrokers(x.inner());
  } else {
    // If not specified then use default
    setBrokers("localhost:9092");
  }
}

void Configuration::extractConversionThreads() {
  if (auto x = find<size_t>("conversion-threads", Json)) {
    ConversionThreads = x.inner();
  } else {
    // If not specified then use default
    ConversionThreads = 1;
  }
}

void Configuration::extractConversionWorkerQueueSize() {
  if (auto x =
      find<size_t>("conversion-worker-queue-size", Json)) {
    ConversionWorkerQueueSize = x.inner();
  } else {
    // If not specified then use default
    ConversionWorkerQueueSize = 1024;
  }
}

void Configuration::extractMainPollInterval() {
  if (auto x = find<int32_t>("main-poll-interval", Json)) {
    MainPollInterval = x.inner();
  } else {
    // If not specified then use default
    MainPollInterval = 500;
  }
}

void Configuration::extractGlobalConverters(std::string &Schema) {
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
              ConverterInts[SettingIt.key()] = SettingIt.value().get<int64_t>();
            }

            if (SettingIt.value().is_string()) {
              ConverterStrings[SettingIt.key()] =
                  SettingIt.value().get<std::string>();
            }
          }
        }
      }
    }
  }
}

void Configuration::extractStatusUri() {
  if (auto x = find<std::string>("status-uri", Json)) {
    auto URIString = x.inner();
    uri::URI URI;
    URI.port = 9092;
    URI.parse(URIString);
    StatusReportURI = URI;
  }
}

void Configuration::setBrokers(std::string Broker) {
  Brokers.clear();
  auto a = split(Broker, ",");
  for (auto &x : a) {
    uri::URI u1;
    u1.require_host_slashes = false;
    u1.parse(x);
    Brokers.push_back(u1);
  }
}

void Configuration::extractKafkaBrokerSettings() {
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
          BrokerSettings.ConfigurationStrings[Key] = Value;
        } else if (Property.value().is_number()) {
          auto Value = Property.value().get<int64_t>();
          LOG(6, "kafka broker config {}: {}", Key, Value);
          BrokerSettings.ConfigurationIntegers[Key] = Value;
        } else {
          LOG(3, "can not understand option: {}", Key);
        }
      }
    }
  }
}

void Configuration::extractStreamSettings() {
  using nlohmann::json;
  if (auto StreamsMaybe = find<json>("streams", Json)) {
    auto Streams = StreamsMaybe.inner();
    if (Streams.is_array()) {
      for (auto const &StreamJson : Streams) {
        StreamSettings Settings;
        // Find the basic information
        extractMappingInfo(StreamJson, Settings.Name, Settings.EpicsProtocol);

        // Find the converters, if present
        if (auto x = find<nlohmann::json>("converter", StreamJson)) {
          if (x.inner().is_object()) {
            Settings.Converters.push_back(extractConverterSettings(x.inner()));
          } else if (x.inner().is_array()) {
            for (auto const &ConverterSettings : x.inner()) {
              Settings.Converters.push_back(extractConverterSettings(ConverterSettings));
            }
          }
        }

        StreamsInfo.push_back(Settings);
      }
    }
  }
}

void Configuration::extractMappingInfo(nlohmann::json const &Mapping,
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
  }
  else {
    // Default is pva
    Protocol = "pva";
  }


}

ConverterSettings Configuration::extractConverterSettings(nlohmann::json const &Mapping) {
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


