#include "EpicsClientMonitor.h"
#include "ChannelRequester.h"
#include "FwdMonitorRequester.h"
#include <atomic>
#include <mutex>
// EPICS 4 supports access via the channel access protocol as well,
// and we need it because some hardware speaks EPICS base.
#include "EpicsPVUpdate.h"
#include "RangeSet.h"
#include "logger.h"
#include <pv/pvAccess.h>
#ifdef _MSC_VER
#include <iso646.h>
#endif
#include "RangeSet.h"

namespace Forwarder {
namespace EpicsClient {

using epics::pvData::PVStructure;
using epics::pvAccess::Channel;

using urlock = std::unique_lock<std::recursive_mutex>;

// Testing alternative
#define RLOCK() urlock lock(mx);

/// Implementation for EPICS client monitor.
class EpicsClientMonitor_impl {
public:
  explicit EpicsClientMonitor_impl(EpicsClientInterface *epics_client)
      : epics_client(epics_client) {}
  ~EpicsClientMonitor_impl() { CLOG(7, 7, "EpicsClientMonitor_implor_impl"); }

  /// Starts the EPICS channel access provider loop and the monitor requester
  /// loop for monitoring EPICS PVs.
  int init(std::string epics_channel_provider_type) {
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

  /// Creates a new monitor requester instance and starts the epics monitoring
  /// loop.
  int monitoring_start() {
    RLOCK();
    if (!channel) {
      LOG(7, "monitoring_start:  want to start but we have no channel");
      return -1;
    }
    LOG(7, "monitoring_start");
    // Leaving it empty seems to be the full channel, including name.  That's
    // good.
    // Can also specify subfields, e.g. "value, timeStamp"  or also
    // "field(value)"
    // We need to be more explicit here for compatibility with channel access.
    std::string request = "field(value,timeStamp,alarm)";
    PVStructure::shared_pointer pvreq =
        epics::pvData::CreateRequest::create()->createRequest(request);
    if (monitor) {
      monitoring_stop();
    }
    monitor_requester.reset(
        new FwdMonitorRequester(epics_client, channel_name));
    monitor = channel->createMonitor(monitor_requester, pvreq);
    if (!monitor) {
      CLOG(3, 1, "could not create EPICS monitor instance");
      return -2;
    }
    return 0;
  }

  /// Stops the EPICS monitor loop in monitor_requester and resets the pointer.
  int monitoring_stop() {
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

  /// Logs that the channel has been destroyed and stops monitoring.
  int channel_destroyed() {
    LOG(4, "channel_destroyed()");
    monitoring_stop();
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
  int emit(std::shared_ptr<FlatBufs::EpicsPVUpdate> up) {
#if TEST_PROVOKE_ERROR == 1
    static std::atomic<int> c1{0};
    if (c1 > 10) {
      epics_client->error_in_epics();
    }
    ++c1;
#endif
    return epics_client->emit(up);
  }

  /// Logging function.
  void error_channel_requester() { LOG(4, "error_channel_requester()"); }

  epics::pvData::MonitorRequester::shared_pointer monitor_requester;
  epics::pvAccess::ChannelProvider::shared_pointer provider;
  epics::pvAccess::ChannelRequester::shared_pointer channel_requester;
  epics::pvAccess::Channel::shared_pointer channel;
  epics::pvData::Monitor::shared_pointer monitor;
  std::recursive_mutex mx;
  std::string channel_name;
  EpicsClientInterface *epics_client = nullptr;
  std::unique_ptr<EpicsClientFactoryInit> factory_init;
};

EpicsClientMonitor::EpicsClientMonitor(
    ChannelInfo &channelInfo,
    std::shared_ptr<
        moodycamel::ConcurrentQueue<std::shared_ptr<FlatBufs::EpicsPVUpdate>>>
        ring)
    : emit_queue(ring) {
  impl.reset(new EpicsClientMonitor_impl(this));
  CLOG(7, 7, "channel_name: {}", channelInfo.channel_name);
  impl->channel_name = channelInfo.channel_name;
  if (impl->init(channelInfo.provider_type) != 0) {
    impl.reset();
    throw std::runtime_error("could not initialize");
  }
}

EpicsClientMonitor::~EpicsClientMonitor() {
  CLOG(7, 6, "EpicsClientMonitorMonitor");
}

int EpicsClientMonitor::stop() { return impl->stop(); }

int EpicsClientMonitor::emit(std::shared_ptr<FlatBufs::EpicsPVUpdate> up) {
  if (!up) {
    CLOG(6, 1, "empty update?");
    // should never happen, ignore
    return 0;
  }
  emit_queue->enqueue(std::move(up));
  return 1;
}

void EpicsClientMonitor::error_in_epics() { status_ = -1; }

#define STRINGIFY2(x) #x
#define STRINGIFY(x) STRINGIFY2(x)

char const *channel_state_name(epics::pvAccess::Channel::ConnectionState x) {
#define DWTN1(N) DWTN2(N, STRINGIFY(N))
#define DWTN2(N, S)                                                            \
  if (x == epics::pvAccess::Channel::ConnectionState::N) {                     \
    return S;                                                                  \
  }
  DWTN1(NEVER_CONNECTED);
  DWTN1(CONNECTED);
  DWTN1(DISCONNECTED);
  DWTN1(DESTROYED);
#undef DWTN1
#undef DWTN2
  return "[unknown]";
}

static std::string
channelInfo(epics::pvAccess::Channel::shared_pointer const &channel) {
  std::ostringstream ss;
  channel->printInfo(ss);
  return ss.str();
}

std::string ChannelRequester::getRequesterName() { return "ChannelRequester"; }

void ChannelRequester::message(std::string const &message,
                               epics::pvData::MessageType messageType) {
  LOG(4, "Message for: {}  msg: {}  msgtype: {}", getRequesterName().c_str(),
      message.c_str(), getMessageTypeName(messageType).c_str());
}

/*
Seems that channel creation is actually a synchronous operation
and that this requester callback is called from the same stack
from which the channel creation was initiated.
*/

void ChannelRequester::channelCreated(epics::pvData::Status const &status,
                                      Channel::shared_pointer const &channel) {
  CLOG(7, 7, "ChannelRequester::channelCreated:  (int)status.isOK(): {}",
       (int)status.isOK());
  if (!status.isOK() or !status.isSuccess()) {
    // quick fix until decided on logging system..
    std::ostringstream s1;
    s1 << status;
    CLOG(4, 5, "WARNING ChannelRequester::channelCreated:  {}",
         s1.str().c_str());
  }
  if (!status.isSuccess()) {
    std::ostringstream s1;
    s1 << status;
    CLOG(3, 2, "ChannelRequester::channelCreated:  failure: {}",
         s1.str().c_str());
    if (channel) {
      std::string cname = channel->getChannelName();
      CLOG(3, 2, "  failure is in channel: {}", cname.c_str());
    }
    epics_client_impl->error_channel_requester();
  }
}

void ChannelRequester::channelStateChange(
    Channel::shared_pointer const &channel, Channel::ConnectionState cstate) {
  CLOG(7, 7, "channel state change: {}", channel_state_name(cstate));
  if (!channel) {
    CLOG(2, 2, "no channel, even though we should have.  state: {}",
         channel_state_name(cstate));
    epics_client_impl->error_channel_requester();
    return;
  }
  if (cstate == Channel::CONNECTED) {
    CLOG(7, 7, "Epics channel connected");
    if (log_level >= 9) {
      LOG(9, "ChannelRequester::channelStateChange  channelinfo: {}",
          channelInfo(channel));
    }
    epics_client_impl->monitoring_start();
  } else if (cstate == Channel::DISCONNECTED) {
    CLOG(7, 6, "Epics channel disconnect");
    epics_client_impl->monitoring_stop();
  } else if (cstate == Channel::DESTROYED) {
    CLOG(7, 6, "Epics channel destroyed");
    epics_client_impl->channel_destroyed();
  } else if (cstate != Channel::CONNECTED) {
    CLOG(3, 3, "Unhandled channel state change: {} {}", cstate,
         channel_state_name(cstate));
    epics_client_impl->error_channel_requester();
  }
}
}
}
