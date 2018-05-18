#pragma once
#include "RangeSet.h"
#include <pv/monitor.h>
namespace BrightnESS {
namespace ForwardEpicsToKafka {
namespace EpicsClient {

class EpicsClient_impl;

class FwdMonitorRequester : public ::epics::pvData::MonitorRequester {
public:
  FwdMonitorRequester(EpicsClient_impl *epics_client_impl,
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
  EpicsClient_impl *epics_client_impl = nullptr;
  RangeSet<uint64_t> seq_data_received;
};
}
}
}