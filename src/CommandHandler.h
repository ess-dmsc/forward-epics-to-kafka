#pragma once
#include "Config.h"
#include "Forwarder.h"
#include "nlohmann/json.hpp"
#include <string>
namespace Forwarder {
class Forwarder;

/// Helper class to provide a callback for the Kafka command listener.
class ConfigCB {
public:
  /// Constructor.
  ///
  /// \param main The owning class which is manipulated from the callback.
  explicit ConfigCB(Forwarder &main);

  /// The callback entry-point.
  ///
  /// \param msg The message to handle.
  void operator()(std::string const &msg);

  /// Extract the command type from the message.
  ///
  /// \param Document The JSON message.
  /// \return The command name.
  static std::string findCommand(nlohmann::json const &Document);

private:
  Forwarder &main;
  void handleCommand(std::string const &Msg);
  void handleCommandAdd(nlohmann::json const &Document);
  void handleCommandStopChannel(nlohmann::json const &Document);
  void handleCommandStopAll();
  void handleCommandExit();
  std::shared_ptr<spdlog::logger> Logger = spdlog::get("ForwarderLogger");
};

} // namespace Forwarder
