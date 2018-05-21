#pragma once
#include "EpicsClient.h"
#include "EpicsClientFactory.h"
#include "ForwarderInfo.h"
#include "Stream.h"
#include <array>
#include <atomic>
#include <pv/pvAccess.h>
#include <string>
#include <vector>

///\file Epics client monitor classes (PIMPL idiom)

namespace BrightnESS {
namespace ForwardEpicsToKafka {
namespace EpicsClient {

using std::array;
using std::vector;

class FwdMonitorRequester;

///\class EpicsClientMonitor_impl
///\brief Implementation for EPICS client monitor.
class EpicsClientMonitor_impl {
public:
  explicit EpicsClientMonitor_impl(EpicsClientMonitor *epics_client);
  ~EpicsClientMonitor_impl();
  int init(std::string epics_channel_provider_type);
  int monitoring_start();
  int monitoring_stop();
  int channel_destroyed();
  int stop();
  int emit(std::unique_ptr<FlatBufs::EpicsPVUpdate>);
  void monitor_requester_error(FwdMonitorRequester *);
  void error_channel_requester();
  epics::pvData::MonitorRequester::shared_pointer monitor_requester;
  epics::pvAccess::ChannelProvider::shared_pointer provider;
  epics::pvAccess::ChannelRequester::shared_pointer channel_requester;
  epics::pvAccess::Channel::shared_pointer channel;
  epics::pvData::Monitor::shared_pointer monitor;
  std::recursive_mutex mx;
  uint64_t teamid = 0;
  uint64_t fwdix = 0;
  std::string channel_name;
  EpicsClientMonitor *epics_client = nullptr;
  std::unique_ptr<EpicsClientFactoryInit> factory_init;
};

///\class EpicsClientMonitor
///\brief Epics client implementation which monitors for PV updates.
class EpicsClientMonitor : EpicsClient {
public:
  EpicsClientMonitor(Stream *stream, std::shared_ptr<ForwarderInfo> finfo,
                     std::string epics_channel_provider_type,
                     std::string channel_name);
  ~EpicsClientMonitor();
  int emit(std::unique_ptr<FlatBufs::EpicsPVUpdate> up);
  int stop();
  void error_in_epics();

private:
  std::shared_ptr<ForwarderInfo> finfo;
  std::unique_ptr<EpicsClientMonitor_impl> impl;
  Stream *stream = nullptr;
};
}
}
}
