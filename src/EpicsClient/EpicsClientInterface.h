#pragma once
#include "EpicsPVUpdate.h"
#include <memory>

namespace Forwarder {
namespace EpicsClient {

enum class ChannelConnectionState : uint8_t {
  UNKNOWN,
  NEVER_CONNECTED,
  CONNECTED,
  DISCONNECTED,
  DESTROYED,
};

std::string toString(ChannelConnectionState const &State);

/// Pure virtual interface for EPICS communication
class EpicsClientInterface {
public:
  virtual ~EpicsClientInterface() = default;
  virtual void emit(std::shared_ptr<FlatBufs::EpicsPVUpdate> Update) = 0;
  virtual int stop() = 0;
  virtual void errorInEpics() = 0;
  virtual int status() = 0;
  virtual std::string getConnectionState() = 0;
  virtual void handleChannelRequesterError(std::string const &){};
  virtual void handleConnectionStateChange(
      ChannelConnectionState /* ConnectionState */){};
  virtual void setServiceID(std::string /* ServiceID */){};
};
}
}
