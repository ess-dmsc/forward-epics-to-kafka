#pragma once
#include "EpicsClientInterface.h"
#include "RangeSet.h"
#include <pv/monitor.h>
namespace Forwarder {
namespace EpicsClient {

class EpicsClientMonitor;

///\class FwdMonitorRequester
///\brief an implementation of an internal epics monitor loop
/// Owned by the epics client monitor, responsible for starting the monitor
/// and creating callbacks.
class FwdMonitorRequester : public ::epics::pvData::MonitorRequester {
public:
  ///\fn FwdMonitorRequester
  ///\brief sets the Requester ID, channel name and pointer to epics client
  FwdMonitorRequester(EpicsClientInterface *epicsClientMonitor,
                      const std::string &channel_name);

  ~FwdMonitorRequester();

  ///\fn getRequesterName
  ///\brief returns the requester ID
  std::string getRequesterName() override;

  ///\fn message
  ///\param msg Message String
  ///\param msg_type Not used, satisfies inheritance from MonitorRequester
  ///\brief logging function from MonitorRequester
  void message(std::string const &msg,
               ::epics::pvData::MessageType msg_type) override;

  ///\fn monitorConnect
  ///\param status The status of the internal epics monitor
  ///\param monitor Reference to the epics monitor
  ///\param structure Not used, used to satisfy inheritance from
  /// MonitorRequester
  ///\brief checks the epics monitor is connected and no errors are returned.
  void
  monitorConnect(::epics::pvData::Status const &status,
                 ::epics::pvData::Monitor::shared_pointer const &monitor,
                 ::epics::pvData::StructureConstPtr const &structure) override;

  ///\fn monitorEvent
  ///\param pointer to the epics monitor loop
  ///\brief create callbacks from the epics pv updates after polling the
  /// monitor.
  /// The callbacks produce epics pv update instances which are then added to
  /// emit_queue via emit()
  void monitorEvent(::epics::pvData::MonitorPtr const &monitor) override;

  ///\fn unlisten
  ///\brief logging method
  void unlisten(::epics::pvData::MonitorPtr const &monitor) override;

private:
  std::string name;
  std::string channel_name;
  uint64_t seq = 0;
  EpicsClientInterface *epics_client = nullptr;
  RangeSet<uint64_t> seq_data_received;
};
}
}
