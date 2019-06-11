#include "ChannelRequester.h"
#include "../logger.h"
#include <pv/pvAccess.h>

namespace Forwarder {
namespace EpicsClient {
static std::string
getChannelInfoString(epics::pvAccess::Channel::shared_pointer const &Channel) {
  std::ostringstream ss;
  Channel->printInfo(ss);
  return ss.str();
}

using epics::pvAccess::Channel;
using epics::pvData::PVStructure;

ChannelRequester::ChannelRequester(EpicsClientInterface *EpicsClient)
    : EpicsClient(EpicsClient) {
  if (EpicsClient == nullptr) {
    throw std::runtime_error("ChannelRequester::ChannelRequester("
                             "EpicsClientInterface *EpicsClient) called with "
                             "EpicsClient == nullptr");
  }
}

std::string ChannelRequester::getRequesterName() { return "ChannelRequester"; }

void ChannelRequester::message(std::string const &Message,
                               epics::pvData::MessageType MessageType) {
  Logger->warn("Message for: {}  msg: {}  msgtype: {}", getRequesterName(),
               Message, getMessageTypeName(MessageType));
}

void ChannelRequester::channelCreated(epics::pvData::Status const &Status,
                                      Channel::shared_pointer const &Channel) {
  // Seems that channel creation is actually a synchronous operation
  // and that this requester callback is called from the same stack
  // from which the channel creation was initiated.
  Logger->trace("ChannelRequester::channelCreated:  (int)status.isOK(): {}",
                (int)Status.isOK());
  if (!Status.isOK() || !Status.isSuccess()) {
    std::string ChannelName;
    if (Channel) {
      ChannelName = Channel->getChannelName();
    }
    std::ostringstream StringStream;
    StringStream << Status;
    auto Message = fmt::format("ChannelRequester::channelCreated:  isOK: {}  "
                               "isSuccess: {}  ChannelName: {}  Message: {}",
                               Status.isOK(), Status.isSuccess(), ChannelName,
                               StringStream.str());
    Logger->warn(Message);
    EpicsClient->handleChannelRequesterError(Message);
  }
}

static ChannelConnectionState
createChannelConnectionState(Channel::ConnectionState EpicsConnectionState) {
  using State = Channel::ConnectionState;
  switch (EpicsConnectionState) {
  case State::NEVER_CONNECTED:
    return ChannelConnectionState::NEVER_CONNECTED;
  case State::CONNECTED:
    return ChannelConnectionState::CONNECTED;
  case State::DISCONNECTED:
    return ChannelConnectionState::DISCONNECTED;
  case State::DESTROYED:
    return ChannelConnectionState::DESTROYED;
  default:
    return ChannelConnectionState::UNKNOWN;
  }
}

void ChannelRequester::channelStateChange(
    Channel::shared_pointer const &Channel,
    Channel::ConnectionState EpicsConnectionState) {
  Logger->trace("channel state change: {}  for: {}",
                toString(EpicsConnectionState), getChannelInfoString(Channel));
  if (!Channel) {
    Logger->error("no channel, even though we should have.  state: {}",
                  toString(EpicsConnectionState));
    EpicsClient->handleChannelRequesterError("No channel given");
    return;
  }
  EpicsClient->handleConnectionStateChange(
      createChannelConnectionState(EpicsConnectionState));
}

std::string
ChannelRequester::toString(const Channel::ConnectionState &ConnectionState) {
  using State = epics::pvAccess::Channel::ConnectionState;
  switch (ConnectionState) {
  case State::NEVER_CONNECTED:
    return "NEVER_CONNECTED";
  case State::CONNECTED:
    return "CONNECTED";
  case State::DISCONNECTED:
    return "DISCONNECTED";
  case State::DESTROYED:
    return "DESTROYED";
  default:
    return "UNKNOWN";
  }
}

} // namespace EpicsClient
} // namespace Forwarder
