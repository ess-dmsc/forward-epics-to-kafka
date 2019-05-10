#pragma once
#include "EpicsClientInterface.h"
#include "logger.h"
#include <pv/pvAccess.h>
#include <pv/pvData.h>
#include <string>

namespace Forwarder {
namespace EpicsClient {

char const *connectionStateName(epics::pvAccess::Channel::ConnectionState x);

/// Provides channel state information for PVs.
class ChannelRequester : public epics::pvAccess::ChannelRequester {
public:
  explicit ChannelRequester(EpicsClientInterface *EpicsClient);

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
      epics::pvAccess::Channel::ConnectionState EpicsConnectionState) override;

private:
  EpicsClientInterface *EpicsClient = nullptr;
  SharedLogger Logger = getLogger();
};
}
} // namespace Forwarder
