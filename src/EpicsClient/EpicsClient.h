#pragma once

#include "ForwarderInfo.h"
#include "Stream.h"
#include <array>
#include <atomic>
#include <memory>
#include <pv/pvAccess.h>
#include <string>
#include <vector>

namespace BrightnESS {
namespace ForwardEpicsToKafka {
namespace EpicsClient {

using std::array;
using std::vector;

class FwdMonitorRequester;

char const *channel_state_name(epics::pvAccess::Channel::ConnectionState x);

struct EpicsClientFactoryInit {
  EpicsClientFactoryInit();
  ~EpicsClientFactoryInit();
  static std::unique_ptr<EpicsClientFactoryInit> factory_init();
  static std::atomic<int> count;
  static std::mutex mxl;
};

class EpicsClient_impl {
public:
  explicit EpicsClient_impl(EpicsClient *epics_client);
  ~EpicsClient_impl();
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
  EpicsClient *epics_client = nullptr;
  std::unique_ptr<EpicsClientFactoryInit> factory_init;
};

class EpicsClient {
public:
  EpicsClient(Stream *stream, std::shared_ptr<ForwarderInfo> finfo,
              std::string epics_channel_provider_type,
              std::string channel_name);
  ~EpicsClient();
  int emit(std::unique_ptr<FlatBufs::EpicsPVUpdate> up);
  int stop();
  void error_in_epics();

private:
  std::string channel_name;
  std::shared_ptr<ForwarderInfo> finfo;
  std::unique_ptr<EpicsClient_impl> impl;
  Stream *stream = nullptr;
};
}
}
}
