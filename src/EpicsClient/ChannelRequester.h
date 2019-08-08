// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

#pragma once
#include "../logger.h"
#include "EpicsClientInterface.h"
#include <pv/pvAccess.h>
#include <pv/pvData.h>
#include <string>

namespace Forwarder {
namespace EpicsClient {

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
