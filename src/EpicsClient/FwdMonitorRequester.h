// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

#pragma once
#include "../RangeSet.h"
#include "../logger.h"
#include "EpicsClientInterface.h"
#include <atomic>
#include <pv/monitor.h>
#include <pv/pvAccess.h>

namespace Forwarder {
namespace EpicsClient {

/// An implementation of an internal epics monitor loop.
/// Owned by the epics client monitor, responsible for starting the monitor
/// and creating callbacks.
class FwdMonitorRequester : public epics::pvAccess::MonitorRequester {
public:
  /// The constructor.
  ///
  /// \param EpicsClientMonitor The PV monitor.
  /// \param ChannelName The PV name.
  FwdMonitorRequester(EpicsClientInterface *EpicsClientMonitor,
                      const std::string &PVName);

  ~FwdMonitorRequester() override;

  /// \return The requester name.
  std::string getRequesterName() override;

  /// Logging function from MonitorRequester.
  ///
  /// \param Message The logging message.
  /// \param MessageType The PV message type.
  void message(std::string const &Message,
               ::epics::pvData::MessageType MessageType) override;

  /// Checks the epics monitor is connected and no errors are returned.
  ///
  /// \param Status The status of the internal epics monitor.
  /// \param Monitor Reference to the epics monitor.
  /// \param Structure The PV structure.
  void
  monitorConnect(::epics::pvData::Status const &Status,
                 ::epics::pvData::Monitor::shared_pointer const &Monitor,
                 ::epics::pvData::StructureConstPtr const &Structure) override;

  /// Create callbacks from the epics pv updates after polling the
  /// monitor.
  /// The callbacks produce epics pv update instances which are then added to
  /// emit_queue via emit()
  ///
  /// \param pointer to the epics monitor loop.
  void monitorEvent(::epics::pvData::MonitorPtr const &Monitor) override;

  /// Logging method.
  ///
  /// \param Monitor The PV monitor.
  void unlisten(::epics::pvData::MonitorPtr const &Monitor) override;

private:
  std::string ChannelName;
  std::string RequesterName;
  EpicsClientInterface *epics_client = nullptr;
  static std::atomic<uint32_t> GlobalIdCounter;
  SharedLogger Logger = getLogger();
};
} // namespace EpicsClient
} // namespace Forwarder
