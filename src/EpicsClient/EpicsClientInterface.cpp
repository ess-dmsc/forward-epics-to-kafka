#include "EpicsClientInterface.h"

namespace Forwarder {
namespace EpicsClient {

std::string toString(ChannelConnectionState const &State) {
  switch (State) {
  case ChannelConnectionState::UNKNOWN:
    return "UNKNOWN";
  case ChannelConnectionState::NEVER_CONNECTED:
    return "NEVER_CONNECTED";
  case ChannelConnectionState::CONNECTED:
    return "CONNECTED";
  case ChannelConnectionState::DISCONNECTED:
    return "DISCONNECTED";
  case ChannelConnectionState::DESTROYED:
    return "DESTROYED";
  default:
    return "UNDEFINED";
  }
}

} // namespace EpicsClient
} // namespace Forwarder
