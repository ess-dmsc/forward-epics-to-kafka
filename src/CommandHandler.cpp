// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

#include "CommandHandler.h"
#include "ConfigParser.h"
#include "Forwarder.h"
#include "json.h"

namespace Forwarder {

ConfigCallback::ConfigCallback(Forwarder &main) : main(main) {}

void ConfigCallback::operator()(std::string const &RawMsg) {
  Logger->debug("Command received: {}", RawMsg);
  try {
    auto Msg = nlohmann::json::parse(RawMsg);
    handleCommand(Msg);
  } catch (nlohmann::json::parse_error const &Error) {
    Logger->error("Could not parse command. Command was {}. Exception was: {}",
                  RawMsg, Error.what());
  } catch (std::exception const &Error) {
    Logger->error("Could not handle command. Command was {}. Exception was: {}",
                  RawMsg, Error.what());
  }
}

void ConfigCallback::handleCommandAdd(nlohmann::json const &Document) {
  ConfigParser Config(Document.dump());
  auto Settings = Config.extractStreamInfo();

  for (auto &Stream : Settings.StreamsInfo) {
    main.addMapping(Stream);
  }
}

void ConfigCallback::handleCommandStopChannel(nlohmann::json const &Document) {
  if (auto ChannelMaybe = find<std::string>("channel", Document)) {
    main.streams.stopChannel(ChannelMaybe.inner());
  }
}

void ConfigCallback::handleCommandStopAll() { main.streams.clearStreams(); }

void ConfigCallback::handleCommandExit() { main.stopForwarding(); }

void ConfigCallback::handleCommand(nlohmann::json const &Msg) {
  CommandType Command = ConfigParser::findCommand(Msg);

  if (Command == CommandType::add) {
    handleCommandAdd(Msg);
  } else if (Command == CommandType::stop_channel) {
    handleCommandStopChannel(Msg);
  } else if (Command == CommandType::stop_all) {
    handleCommandStopAll();
  } else if (Command == CommandType::exit) {
    handleCommandExit();
  } else {
    throw std::runtime_error("Unknown command type received");
  }
}

} // namespace Forwarder
