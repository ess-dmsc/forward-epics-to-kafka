#include <pv/pvAccess.h>
#include <pv/pvData.h>
#include <string>

namespace Forwarder {
namespace EpicsClient {

char const *channelStateName(epics::pvAccess::Channel::ConnectionState x);

class EpicsClientMonitor_impl;

/// Provides channel state information for PVs.
class ChannelRequester : public epics::pvAccess::ChannelRequester {
public:
  explicit ChannelRequester(EpicsClientMonitor_impl *EpicsClientImpl)
      : EpicsClientImpl(EpicsClientImpl){};

  std::string getRequesterName() override;

  /// Logs the channel status message.
  void message(std::string const &Message,
               epics::pvData::MessageType MessageType) override;

  /// Checks the channel has been created and does not return an error
  /// code status.
  ///
  /// \param Status The status of the channel connection.
  /// \param Channel The pointer to the channel(PV)
  void channelCreated(
      const epics::pvData::Status &Status,
      epics::pvAccess::Channel::shared_pointer const &Channel) override;

  /// Checks the channel state and sets the epics client channel status.
  void channelStateChange(
      epics::pvAccess::Channel::shared_pointer const &Channel,
      epics::pvAccess::Channel::ConnectionState ConnectionState) override;

private:
  EpicsClientMonitor_impl *EpicsClientImpl = nullptr;
};
}
}
