#include "ChannelRequester.h"
#include "EpicsClientMonitor.h"
#include "logger.h"

namespace Forwarder {
namespace EpicsClient {

using epics::pvAccess::Channel;

#define STRINGIFY2(x) #x
#define STRINGIFY(x) STRINGIFY2(x)

char const *channel_state_name(epics::pvAccess::Channel::ConnectionState x) {
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
channelInfo(epics::pvAccess::Channel::shared_pointer const &channel) {
  std::ostringstream ss;
  channel->printInfo(ss);
  return ss.str();
}

ChannelRequester::ChannelRequester(EpicsClientMonitor_impl *epics_client_impl)
    : epics_client_impl(epics_client_impl) {}

std::string ChannelRequester::getRequesterName() { return "ChannelRequester"; }

void ChannelRequester::message(std::string const &message,
                               epics::pvData::MessageType messageType) {
  LOG(4, "Message for: {}  msg: {}  msgtype: {}", getRequesterName().c_str(),
      message.c_str(), getMessageTypeName(messageType).c_str());
}

/*
Seems that channel creation is actually a synchronous operation
and that this requester callback is called from the same stack
from which the channel creation was initiated.
*/

void ChannelRequester::channelCreated(epics::pvData::Status const &status,
                                      Channel::shared_pointer const &channel) {
  CLOG(7, 7, "ChannelRequester::channelCreated:  (int)status.isOK(): {}",
       (int)status.isOK());
  if (!status.isOK() or !status.isSuccess()) {
    // quick fix until decided on logging system..
    std::ostringstream s1;
    s1 << status;
    CLOG(4, 5, "WARNING ChannelRequester::channelCreated:  {}",
         s1.str().c_str());
  }
  if (!status.isSuccess()) {
    std::ostringstream s1;
    s1 << status;
    CLOG(3, 2, "ChannelRequester::channelCreated:  failure: {}",
         s1.str().c_str());
    if (channel) {
      std::string cname = channel->getChannelName();
      CLOG(3, 2, "  failure is in channel: {}", cname.c_str());
    }
    epics_client_impl->error_channel_requester();
  }
}

void ChannelRequester::channelStateChange(
    Channel::shared_pointer const &channel, Channel::ConnectionState cstate) {
  CLOG(7, 7, "channel state change: {}", channel_state_name(cstate));
  if (!channel) {
    CLOG(2, 2, "no channel, even though we should have.  state: {}",
         channel_state_name(cstate));
    epics_client_impl->error_channel_requester();
    return;
  }
  if (cstate == Channel::CONNECTED) {
    CLOG(7, 7, "Epics channel connected");
    if (log_level >= 9) {
      LOG(9, "ChannelRequester::channelStateChange  channelinfo: {}",
          channelInfo(channel));
    }
    epics_client_impl->monitoring_start();
  } else if (cstate == Channel::DISCONNECTED) {
    CLOG(7, 6, "Epics channel disconnect");
    epics_client_impl->monitoring_stop();
  } else if (cstate == Channel::DESTROYED) {
    CLOG(7, 6, "Epics channel destroyed");
    epics_client_impl->channel_destroyed();
  } else if (cstate != Channel::CONNECTED) {
    CLOG(3, 3, "Unhandled channel state change: {} {}", cstate,
         channel_state_name(cstate));
    epics_client_impl->error_channel_requester();
  }
}
}
}
