#include <pv/pvAccess.h>
#include <pv/pvData.h>
#include <string>

namespace Forwarder {
namespace EpicsClient {

char const *channel_state_name(epics::pvAccess::Channel::ConnectionState x);

class EpicsClientMonitor_impl;

///\Class ChannelRequester
///\brief provides channel state information for PVs
class ChannelRequester : public epics::pvAccess::ChannelRequester {
public:
  ChannelRequester(EpicsClientMonitor_impl *epics_client_impl)
      : epics_client_impl(epics_client_impl){};
  // From class pvData::Requester
  std::string getRequesterName() override;

  ///\fn message
  ///\brief logs the channel status message.
  void message(std::string const &message,
               epics::pvData::MessageType messageType) override;

  ///\fn channelCreated
  ///\brief checks the channel has been created and does not return an error
  ///code status.
  void channelCreated(
      const epics::pvData::Status &status,
      epics::pvAccess::Channel::shared_pointer const &channel) override;

  ///\fn channelStateChange
  ///\brief checks channel state and sets the epics client channel status.
  void channelStateChange(
      epics::pvAccess::Channel::shared_pointer const &channel,
      epics::pvAccess::Channel::ConnectionState connectionState) override;

private:
  EpicsClientMonitor_impl *epics_client_impl = nullptr;
};
}
}
