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
#include "json.h"
#include "logger.h"
#include <nlohmann/json.hpp>

namespace Forwarder {

ConfigCallback::ConfigCallback(Forwarder &main) : main(main) {}

void ConfigCallback::operator()(std::string const &msg) {
  Logger->debug("Command received: {}", msg);
  try {
    handleCommand(msg);
  } catch (nlohmann::json::parse_error const &e) {
    Logger->error("Could not parse command. Command was {}. Exception was: {}",
                  msg, e.what());
  } catch (...) {
    Logger->error("Could not handle command: {}", msg);
  }
}

void ConfigCallback::handleCommandAdd(nlohmann::json const &Document) {
  // Use instance of ConfigParser to extract stream info.
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

void ConfigCallback::handleCommand(std::string const &Msg) {
  using nlohmann::json;
  auto Document = json::parse(Msg);

  std::string Command = ConfigParser::findCommand(Document);

  if (Command == "add") {
    handleCommandAdd(Document);
  } else if (Command == "stop_channel") {
    handleCommandStopChannel(Document);
  } else if (Command == "stop_all") {
    handleCommandStopAll();
  } else if (Command == "exit") {
    handleCommandExit();
  } else {
    Logger->info("Cannot understand command: {}", Command);
  }
}

} // namespace Forwarder
