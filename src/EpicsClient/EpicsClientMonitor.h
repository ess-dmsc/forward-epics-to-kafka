#pragma once
#include "EpicsClientFactory.h"
#include "EpicsClientInterface.h"
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
class EpicsClientMonitor;

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
  int emit(std::shared_ptr<FlatBufs::EpicsPVUpdate>);
  void error_channel_requester();
  epics::pvData::MonitorRequester::shared_pointer monitor_requester;
  epics::pvAccess::ChannelProvider::shared_pointer provider;
  epics::pvAccess::ChannelRequester::shared_pointer channel_requester;
  epics::pvAccess::Channel::shared_pointer channel;
  epics::pvData::Monitor::shared_pointer monitor;
  std::recursive_mutex mx;
  std::string channel_name;
  EpicsClientMonitor *epics_client = nullptr;
  std::unique_ptr<EpicsClientFactoryInit> factory_init;
};

///\class EpicsClientMonitor
///\brief Epics client implementation which monitors for PV updates.
class EpicsClientMonitor : public EpicsClientInterface {
public:
  explicit EpicsClientMonitor(
      ChannelInfo &channelInfo,
      std::shared_ptr<Ring<std::shared_ptr<FlatBufs::EpicsPVUpdate>>> ring);
  ~EpicsClientMonitor() override;
  int emit(std::shared_ptr<FlatBufs::EpicsPVUpdate> up) override;
  int stop() override;
  void error_in_epics() override;
  int status() override { return status_; };

private:
  std::unique_ptr<EpicsClientMonitor_impl> impl;
  std::shared_ptr<Ring<std::shared_ptr<FlatBufs::EpicsPVUpdate>>> emit_queue;
  std::atomic<int> status_{0};
};
}
}
}
