#pragma once
#include "ChannelRequester.h"
#include "FwdMonitorRequester.h"
#include <pv/pvAccess.h>
#include <pv/pvData.h>

namespace Forwarder {
namespace EpicsClient {

using urlock = std::unique_lock<std::recursive_mutex>;
#define RLOCK() urlock lock(mx);

/// Implementation for EPICS client monitor.
class EpicsClientMonitorImpl {
public:
  explicit EpicsClientMonitorImpl(EpicsClientInterface *EpicsClient)
      : EpicsClient(EpicsClient) {}
  ~EpicsClientMonitorImpl() { LOG(Sev::Debug, "EpicsClientMonitorImpl"); }

  /// Starts the EPICS channel access provider loop and the monitor requester
  /// loop for monitoring EPICS PVs.
  int init(std::string const &epics_channel_provider_type) {
    factory_init = EpicsClientFactoryInit::factory_init();
    {
      RLOCK();
      provider = ::epics::pvAccess::getChannelProviderRegistry()->getProvider(
          epics_channel_provider_type);
      if (!provider) {
        LOG(Sev::Error, "Can not initialize provider");
        return 1;
      }
      channel_requester.reset(new ChannelRequester(EpicsClient));
      channel = provider->createChannel(channel_name, channel_requester);
    }
    return 0;
  }

  /// Creates a new monitor requester instance and starts the epics monitoring
  /// loop.
  int monitoringStart() {
    RLOCK();
    if (!channel) {
      LOG(7, "monitoringStart:  want to start but we have no channel");
      return -1;
    }
    LOG(Sev::Debug, "monitoringStart");
    // Leaving it empty seems to be the full channel, including name.  That's
    // good.
    // Can also specify subfields, e.g. "value, timeStamp"  or also
    // "field(value)"
    // We need to be more explicit here for compatibility with channel access.
    std::string request = "field(value,timeStamp,alarm)";
    epics::pvData::PVStructure::shared_pointer pvreq =
        epics::pvData::CreateRequest::create()->createRequest(request);
    if (monitor) {
      monitoringStop();
    }
    monitor_requester.reset(new FwdMonitorRequester(EpicsClient, channel_name));
    monitor = channel->createMonitor(monitor_requester, pvreq);
    if (!monitor) {
      LOG(Sev::Warning, "could not create EPICS monitor instance");
      return -2;
    }
    return 0;
  }

  /// Stops the EPICS monitor loop in monitor_requester and resets the pointer.
  int monitoringStop() {
    RLOCK();
    LOG(Sev::Debug, "monitoringStop");
    if (monitor) {
      monitor->stop();
      monitor->destroy();
    }
    monitor_requester.reset();
    monitor.reset();
    return 0;
  }

  /// Stops the EPICS monitor loop.
  int stop() {
    RLOCK();
    if (monitor) {
      monitor->stop();
      monitor->destroy();
    }
    if (channel) {
      channel->destroy();
    }
    monitor.reset();
    channel.reset();
    return 0;
  }

  /// Pushes update to the emit_queue ring buffer which is owned by a stream.
  int emit(std::shared_ptr<FlatBufs::EpicsPVUpdate> const &Update) {
#if TEST_PROVOKE_ERROR == 1
    static std::atomic<int> c1{0};
    if (c1 > 10) {
      EpicsClient->error_in_epics();
    }
    ++c1;
#endif
    return EpicsClient->emit(Update);
  }

  /// Logging function.
  static void error_channel_requester() {
    LOG(Sev::Warning, "error_channel_requester()");
  }

  epics::pvData::MonitorRequester::shared_pointer monitor_requester;
  epics::pvAccess::ChannelProvider::shared_pointer provider;
  epics::pvAccess::ChannelRequester::shared_pointer channel_requester;
  epics::pvAccess::Channel::shared_pointer channel;
  epics::pvData::Monitor::shared_pointer monitor;
  std::recursive_mutex mx;
  std::string channel_name;
  EpicsClientInterface *EpicsClient = nullptr;
  std::unique_ptr<EpicsClientFactoryInit> factory_init;
};

} // namespace EpicsClient
} // namespace Forwarder
