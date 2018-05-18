#pragma once
#include "Main.h"
#include "Config.h"
#include <string>

namespace BrightnESS {
namespace ForwardEpicsToKafka {

/// \brief Helper class to provide a callback for the Kafka command listener.
class ConfigCB : public Config::Callback {
public:
  explicit ConfigCB(Main &main);
  // This is called from the same thread as the main watchdog because the
  // code below calls the config poll which in turn calls this callback.
  void operator()(std::string const &msg) override;
  void handleCommand(std::string const &Msg);
  void handleCommandAdd(nlohmann::json const &Document);
  void handleCommandStopChannel(nlohmann::json const &Document);
  void handleCommandStopAll();
  void handleCommandExit();
  std::string findCommand(nlohmann::json const &Document);

private:
  Main &main;
};

}
}
