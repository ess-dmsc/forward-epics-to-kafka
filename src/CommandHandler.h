#pragma once
#include "Config.h"
#include "Main.h"
#include <string>

namespace BrightnESS {
namespace ForwardEpicsToKafka {

/// Helper class to provide a callback for the Kafka command listener.
class ConfigCB : public Config::Callback {
public:
  /// Constructor.
  ///
  /// \param main The owning class which is manipulated from the callback.
  explicit ConfigCB(Main &main);

  /// The callback.
  ///
  /// \param msg The message to handle.
  void operator()(std::string const &msg) override;

  std::string findCommand(nlohmann::json const &Document);

private:
  Main &main;
  void handleCommand(std::string const &Msg);
  void handleCommandAdd(nlohmann::json const &Document);
  void handleCommandStopChannel(nlohmann::json const &Document);
  void handleCommandStopAll();
  void handleCommandExit();
};

} // namespace ForwardEpicsToKafka
} // namespace BrightnESS
