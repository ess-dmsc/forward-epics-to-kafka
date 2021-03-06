// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

#include "ConfigParser.h"
#include "Forwarder.h"
#include "json.h"
#include "logger.h"
#include <iostream>

namespace Forwarder {

static std::vector<std::string> split(const std::string &Brokers) {
  std::vector<std::string> ret;
  std::string::size_type i1 = 0;
  while (true) {
    auto i2 = Brokers.find(',', i1);
    if (i2 == std::string::npos) {
      break;
    }
    if (i2 > i1) {
      ret.push_back(Brokers.substr(i1, i2 - i1));
    }
    i1 = i2 + 1;
  }
  if (i1 != Brokers.size()) {
    ret.push_back(Brokers.substr(i1));
  }
  return ret;
}

ConfigParser::ConfigParser(const std::string &RawJson)
    : Json(nlohmann::json::parse(RawJson)) {}

ConfigSettings ConfigParser::extractStreamInfo() {
  ConfigSettings Settings{};
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
  return Settings;
}

void ConfigParser::setBrokers(std::string const &Brokers,
                              ConfigSettings &Settings) {
  Settings.Brokers.clear();
  std::vector<std::string> ret = split(Brokers);
  for (auto &x : ret) {
    URI u1(x);
    Settings.Brokers.push_back(u1);
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

  Protocol = "pva";
  if (auto ChannelProviderTypeMaybe =
          find<std::string>("channel_provider_type", Mapping)) {
    auto TempProtocol = ChannelProviderTypeMaybe.inner();
    if (TempProtocol == "ca" or TempProtocol == "pva") {
      Protocol = TempProtocol;
    } else {
      getLogger()->warn(
          R"xx(When setting up stream "{}": Does not recognise protocol of type "{}", using the default ("{}").)xx",
          Channel, TempProtocol, Protocol);
    }
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

CommandType ConfigParser::findCommand(nlohmann::json const &Document) {
  if (auto CommandMaybe = find<std::string>("cmd", Document)) {
    if (CommandMaybe.inner() == "add") {
      return CommandType::add;
    } else if (CommandMaybe.inner() == "stop_channel") {
      return CommandType::stop_channel;
    } else if (CommandMaybe.inner() == "stop_all") {
      return CommandType::stop_all;
    } else if (CommandMaybe.inner() == "exit") {
      return CommandType::exit;
    }
  }
  return CommandType::unknown;
}
} // namespace Forwarder
