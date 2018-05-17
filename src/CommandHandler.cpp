#include "CommandHandler.h"
#include <nlohmann/json.hpp>
#include "helper.h"
#include "json.h"
#include "logger.h"
#include "ConfigParser.h"

namespace BrightnESS {
namespace ForwardEpicsToKafka {

ConfigCB::ConfigCB(Main &main) : main(main) {}

void ConfigCB::operator()(std::string const &msg) {
  LOG(7, "Command received: {}", msg);
  try {
    handleCommand(msg);
  } catch (...) {
    LOG(3, "Command does not look like valid json: {}", msg);
  }
}

void ConfigCB::handleCommandAdd(nlohmann::json const &Document) {
  // Use instance of ConfigParser to extract stream info.
  ConfigParser Config;
  Config.setJsonFromString(Document.dump());
  Config.extractConfiguration();

  for (auto & Stream : Config.Settings.StreamsInfo) {
    main.addMapping(Stream);
  }
}

void ConfigCB::handleCommandStopChannel(nlohmann::json const &Document) {
  if (auto ChannelMaybe = find<std::string>("channel", Document)) {
    main.streams.channel_stop(ChannelMaybe.inner());
  }
}

void ConfigCB::handleCommandStopAll() { main.streams.streams_clear(); }

void ConfigCB::handleCommandExit() { main.stopForwarding(); }

void ConfigCB::handleCommand(std::string const &Msg) {
  using nlohmann::json;
  auto Document = json::parse(Msg);

  std::string Command = findCommand(Document);

  if (Command == "add") {
    handleCommandAdd(Document);
  } else if (Command == "stop_channel") {
    handleCommandStopChannel(Document);
  } else if (Command == "stop_all") {
    handleCommandStopAll();
  } else if (Command == "exit") {
    handleCommandExit();
  } else {
    LOG(6, "Cannot understand command: {}", Command);
  }
}

std::string ConfigCB::findCommand(nlohmann::json const &Document) {
  if (auto CommandMaybe = find<std::string>("cmd", Document)) {
    return CommandMaybe.inner();
  }

  return std::string();
}

}
}

