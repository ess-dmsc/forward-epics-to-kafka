#include "EpicsClientMonitor.h"
#include "ChannelRequester.h"
#include "FwdMonitorRequester.h"
#include <atomic>
#include <mutex>
// EPICS 4 supports access via the channel access protocol as well,
// and we need it because some hardware speaks EPICS base.
#include "RangeSet.h"
#include "epics-to-fb.h"
#include "logger.h"
#ifdef _MSC_VER
#include <iso646.h>
#endif
#include "RangeSet.h"

namespace BrightnESS {
namespace ForwardEpicsToKafka {
namespace EpicsClient {

using epics::pvData::PVStructure;

using urlock = std::unique_lock<std::recursive_mutex>;

// Testing alternative
#define RLOCK() urlock lock(mx);

EpicsClientMonitor_impl::EpicsClientMonitor_impl(
    EpicsClientMonitor *epics_client)
    : epics_client(epics_client) {}

int EpicsClientMonitor_impl::init(std::string epics_channel_provider_type) {
  factory_init = EpicsClientFactoryInit::factory_init();
  {
    RLOCK();
    provider = ::epics::pvAccess::getChannelProviderRegistry()->getProvider(
        epics_channel_provider_type);
    if (!provider) {
      CLOG(3, 1, "Can not initialize provider");
      return 1;
    }
    channel_requester.reset(new ChannelRequester(this));
    channel = provider->createChannel(channel_name, channel_requester);
  }
  return 0;
}

int EpicsClientMonitor_impl::stop() {
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

int EpicsClientMonitor_impl::monitoring_stop() {
  RLOCK();
  LOG(7, "monitoring_stop");
  if (monitor) {
    monitor->stop();
    monitor->destroy();
  }
  monitor_requester.reset();
  monitor.reset();
  return 0;
}

int EpicsClientMonitor_impl::monitoring_start() {
  RLOCK();
  if (!channel) {
    LOG(7, "monitoring_start:  want to start but we have no channel");
    return -1;
  }
  LOG(7, "monitoring_start");
  // Leaving it empty seems to be the full channel, including name.  That's
  // good.
  // Can also specify subfields, e.g. "value, timeStamp"  or also "field(value)"
  // We need to be more explicit here for compatibility with channel access.
  std::string request = "field(value,timeStamp,alarm)";
  PVStructure::shared_pointer pvreq =
      epics::pvData::CreateRequest::create()->createRequest(request);
  if (monitor) {
    monitoring_stop();
  }
  monitor_requester.reset(new FwdMonitorRequester(epics_client, channel_name));
  monitor = channel->createMonitor(monitor_requester, pvreq);
  if (!monitor) {
    CLOG(3, 1, "could not create EPICS monitor instance");
    return -2;
  }
  return 0;
}

EpicsClientMonitor_impl::~EpicsClientMonitor_impl() {
  CLOG(7, 7, "EpicsClientMonitor_implor_impl");
}

int EpicsClientMonitor_impl::emit(std::unique_ptr<FlatBufs::EpicsPVUpdate> up) {
#if TEST_PROVOKE_ERROR == 1
  static std::atomic<int> c1{0};
  if (c1 > 10) {
    epics_client->error_in_epics();
  }
  ++c1;
#endif
  return epics_client->emit(std::move(up));
}

int EpicsClientMonitor_impl::channel_destroyed() {
  LOG(4, "channel_destroyed()");
  monitoring_stop();
  return 0;
}

void EpicsClientMonitor_impl::error_channel_requester() {
  LOG(4, "error_channel_requester()");
}

EpicsClientMonitor::EpicsClientMonitor(std::string epics_channel_provider_type,
                                       std::string channel_name, Ring<std::unique_ptr<FlatBufs::EpicsPVUpdate>>* ring)
    : emit_queue(ring) {
  impl.reset(new EpicsClientMonitor_impl(this));
  CLOG(7, 7, "channel_name: {}", channel_name);
  impl->channel_name = channel_name;
  if (impl->init(epics_channel_provider_type) != 0) {
    impl.reset();
    throw std::runtime_error("could not initialize");
  }
}

EpicsClientMonitor::~EpicsClientMonitor() {
  CLOG(7, 6, "EpicsClientMonitorMonitor");
}

int EpicsClientMonitor::stop() { return impl->stop(); }

int EpicsClientMonitor::emit(std::unique_ptr<FlatBufs::EpicsPVUpdate> up) {
  if (!up) {
    CLOG(6, 1, "empty update?");
    // should never happen, ignore
    return 0;
  }

  emit_queue->push_enlarge(up);

  // here we are, saying goodbye to a good buffer
  up.reset();
  return 1;
}

void EpicsClientMonitor::error_in_epics() { _status = -1;}
}
}
}
