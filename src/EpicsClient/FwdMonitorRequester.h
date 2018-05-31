#pragma once
#include "RangeSet.h"
#include "EpicsClientInterface.h"
#include <pv/monitor.h>
namespace BrightnESS {
namespace ForwardEpicsToKafka {
namespace EpicsClient {

class EpicsClientMonitor;

///\class FwdMonitorRequester
///\brief an implementation of an internal epics monitor loop
class FwdMonitorRequester : public ::epics::pvData::MonitorRequester {
public:
  FwdMonitorRequester(EpicsClientInterface *epicsClientMonitor,
                      const std::string &channel_name);
  ~FwdMonitorRequester();
  std::string getRequesterName() override;
  void message(std::string const &msg,
               ::epics::pvData::MessageType msg_type) override;

  void
  monitorConnect(::epics::pvData::Status const &status,
                 ::epics::pvData::Monitor::shared_pointer const &monitor,
                 ::epics::pvData::StructureConstPtr const &structure) override;

  void monitorEvent(::epics::pvData::MonitorPtr const &monitor) override;
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
}