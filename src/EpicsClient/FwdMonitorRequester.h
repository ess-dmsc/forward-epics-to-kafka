#pragma once
#include "EpicsClientInterface.h"
#include "RangeSet.h"
#include <pv/monitor.h>
namespace Forwarder {
namespace EpicsClient {

class EpicsClientMonitor;

/// An implementation of an internal epics monitor loop.
/// Owned by the epics client monitor, responsible for starting the monitor
/// and creating callbacks.
class FwdMonitorRequester : public ::epics::pvData::MonitorRequester {
public:
  /// Sets the Requester ID, channel name and pointer to epics client.
  FwdMonitorRequester(EpicsClientInterface *EpicsClientMonitor,
                      const std::string &ChannelName);

  ~FwdMonitorRequester() override;

  /// returns the requester ID.
  std::string getRequesterName() override;

  /// Logging function from MonitorRequester.
  ///
  ///\param Message The logging message.
  ///\param MessageType Not used, satisfies inheritance from MonitorRequester.
  void message(std::string const &Message,
               ::epics::pvData::MessageType MessageType) override;

  /// Checks the epics monitor is connected and no errors are returned.
  ///
  ///\param Status The status of the internal epics monitor.
  ///\param Monitor Reference to the epics monitor.
  ///\param Structure Not used, used to satisfy inheritance from
  /// MonitorRequester.
  void
  monitorConnect(::epics::pvData::Status const &Status,
                 ::epics::pvData::Monitor::shared_pointer const &Monitor,
                 ::epics::pvData::StructureConstPtr const &Structure) override;

  /// Create callbacks from the epics pv updates after polling the
  /// monitor.
  /// The callbacks produce epics pv update instances which are then added to
  /// emit_queue via emit()
  ///
  ///\param pointer to the epics monitor loop.
  void monitorEvent(::epics::pvData::MonitorPtr const &Monitor) override;

  /// Logging method.
  void unlisten(::epics::pvData::MonitorPtr const &Monitor) override;

private:
  std::string name;
  std::string channel_name;
  uint64_t seq = 0;
  EpicsClientInterface *epics_client = nullptr;
  RangeSet<uint64_t> seq_data_received;
};
}
}
