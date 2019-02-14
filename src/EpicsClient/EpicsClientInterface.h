#pragma once
#include "EpicsPVUpdate.h"
#include <memory>

namespace Forwarder {
namespace EpicsClient {

/// Pure virtual interface for EPICS communication
class EpicsClientInterface {
public:
  virtual ~EpicsClientInterface() = default;
  virtual int emit(std::shared_ptr<FlatBufs::EpicsPVUpdate> Update) = 0;
  virtual int stop() = 0;
  virtual void errorInEpics() = 0;
  virtual int status() = 0;
  virtual std::string getConnectionStatus() = 0;
  virtual void handleChannelRequesterError(std::string const &){};
  virtual void handleConnectionStateChange(std::string const &){};
};
}
}
