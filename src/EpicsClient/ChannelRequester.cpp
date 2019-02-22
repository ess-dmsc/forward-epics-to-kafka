#include "ChannelRequester.h"
#include "logger.h"
#include <pv/pvAccess.h>

namespace Forwarder {
namespace EpicsClient {

#define STRINGIFY2(x) #x
#define STRINGIFY(x) STRINGIFY2(x)

char const *connectionStateName(epics::pvAccess::Channel::ConnectionState x) {
#define DWTN1(N) DWTN2(N, STRINGIFY(N))
#define DWTN2(N, S)                                                            \
  if (x == epics::pvAccess::Channel::ConnectionState::N) {                     \
    return S;                                                                  \
  }
  DWTN1(NEVER_CONNECTED);
  DWTN1(CONNECTED);
  DWTN1(DISCONNECTED);
  DWTN1(DESTROYED);
#undef DWTN1
#undef DWTN2
  return "[unknown]";
}

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
  LOG(Sev::Warning, "Message for: {}  msg: {}  msgtype: {}", getRequesterName(),
      Message, getMessageTypeName(MessageType));
}

void ChannelRequester::channelCreated(epics::pvData::Status const &Status,
                                      Channel::shared_pointer const &Channel) {
  // Seems that channel creation is actually a synchronous operation
  // and that this requester callback is called from the same stack
  // from which the channel creation was initiated.
  LOG(Sev::Debug, "ChannelRequester::channelCreated:  (int)status.isOK(): {}",
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
    LOG(Sev::Warning, "{}", Message);
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
  LOG(Sev::Debug, "channel state change: {}  for: {}",
      connectionStateName(EpicsConnectionState), getChannelInfoString(Channel));
  if (!Channel) {
    LOG(Sev::Error, "no channel, even though we should have.  state: {}",
        connectionStateName(EpicsConnectionState));
    EpicsClient->handleChannelRequesterError("No channel given");
    return;
  }
  EpicsClient->handleConnectionStateChange(
      createChannelConnectionState(EpicsConnectionState));
}

} // namespace EpicsClient
} // namespace Forwarder
