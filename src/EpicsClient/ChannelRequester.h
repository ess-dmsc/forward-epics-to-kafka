#include <pv/pvAccess.h>
#include <pv/pvData.h>
#include <string>

namespace Forwarder {
namespace EpicsClient {

char const *channel_state_name(epics::pvAccess::Channel::ConnectionState x);

class EpicsClientMonitor_impl;

/// Provides channel state information for PVs.
class ChannelRequester : public epics::pvAccess::ChannelRequester {
public:
  explicit ChannelRequester(EpicsClientMonitor_impl *epics_client_impl)
      : epics_client_impl(epics_client_impl){};
  std::string getRequesterName() override;

  /// Logs the channel status message.
  void message(std::string const &message,
               epics::pvData::MessageType messageType) override;

  /// Checks the channel has been created and does not return an error
  /// code status.
  ///
  /// \param status The status of the channel connection.
  /// \param channel The pointer to the channel(PV)
  void channelCreated(
      const epics::pvData::Status &status,
      epics::pvAccess::Channel::shared_pointer const &channel) override;

  /// Checks the channel state and sets the epics client channel status.
  void channelStateChange(
      epics::pvAccess::Channel::shared_pointer const &channel,
      epics::pvAccess::Channel::ConnectionState connectionState) override;

private:
  EpicsClientMonitor_impl *epics_client_impl = nullptr;
};
}
}
