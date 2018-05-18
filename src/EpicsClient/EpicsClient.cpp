#include "EpicsClient.h"
#include "ChannelRequester.h"
#include "FwdMonitorRequester.h"
#include <atomic>
#include <mutex>
#include <pv/pvAccess.h>
#include <pv/pvData.h>
// For epics::pvAccess::ClientFactory::start()
#include <pv/clientFactory.h>
// EPICS 4 supports access via the channel access protocol as well,
// and we need it because some hardware speaks EPICS base.
#include "RangeSet.h"
#include "epics-to-fb.h"
#include "logger.h"
#include <pv/caProvider.h>
#ifdef _MSC_VER
#include <iso646.h>
#endif
#include "RangeSet.h"

namespace BrightnESS {
namespace ForwardEpicsToKafka {
namespace EpicsClient {

using epics::pvData::PVStructure;

using std::mutex;
using ulock = std::unique_lock<mutex>;
using urlock = std::unique_lock<std::recursive_mutex>;

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

// Testing alternative
#define RLOCK() urlock lock(mx);

class EpicsClient_impl;

class ActionOnChannel {
public:
  ActionOnChannel(EpicsClient_impl *epics_client_impl)
      : epics_client_impl(epics_client_impl) {}
  virtual void
  operator()(epics::pvAccess::Channel::shared_pointer const &channel) {
    LOG(2, "[EMPTY ACTION]");
  };
  EpicsClient_impl *epics_client_impl;
};



std::atomic<int> EpicsClientFactoryInit::count{0};
std::mutex EpicsClientFactoryInit::mxl;
std::unique_ptr<EpicsClientFactoryInit> EpicsClientFactoryInit::factory_init() {
  return std::unique_ptr<EpicsClientFactoryInit>(new EpicsClientFactoryInit);
}
EpicsClientFactoryInit::EpicsClientFactoryInit() {
  CLOG(7, 7, "EpicsClientFactoryInit");
  ulock lock(mxl);
  auto c = count++;
  if (c == 0) {
    CLOG(6, 6, "START  Epics factories");
    ::epics::pvAccess::ClientFactory::start();
    ::epics::pvAccess::ca::CAClientFactory::start();
  }
}
EpicsClientFactoryInit::~EpicsClientFactoryInit() {
  CLOG(7, 7, "~EpicsClientFactoryInit");
  ulock lock(mxl);
  auto c = --count;
  if (c < 0) {
    LOG(0, "Reference count {} is not consistent, should never happen, but "
           "ignoring for now.",
        c);
    c = 0;
  }
  if (c == 0) {
    CLOG(7, 6, "STOP   Epics factories");
    ::epics::pvAccess::ClientFactory::stop();
    ::epics::pvAccess::ca::CAClientFactory::stop();
  }
}


EpicsClient_impl::EpicsClient_impl(EpicsClient *epics_client)
    : epics_client(epics_client) {}

int EpicsClient_impl::init(std::string epics_channel_provider_type) {
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

int EpicsClient_impl::stop() {
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

int EpicsClient_impl::monitoring_stop() {
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

int EpicsClient_impl::monitoring_start() {
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
  monitor_requester.reset(new FwdMonitorRequester(this, channel_name));
  monitor = channel->createMonitor(monitor_requester, pvreq);
  if (!monitor) {
    CLOG(3, 1, "could not create EPICS monitor instance");
    return -2;
  }
  return 0;
}

EpicsClient_impl::~EpicsClient_impl() { CLOG(7, 7, "~EpicsClient_impl"); }

int EpicsClient_impl::emit(std::unique_ptr<FlatBufs::EpicsPVUpdate> up) {
#if TEST_PROVOKE_ERROR == 1
  static std::atomic<int> c1{0};
  if (c1 > 10) {
    epics_client->error_in_epics();
  }
  ++c1;
#endif
  return epics_client->emit(std::move(up));
}

void EpicsClient_impl::monitor_requester_error(FwdMonitorRequester *ptr) {
  LOG(4, "monitor_requester_error()");
  epics_client->error_in_epics();
}

int EpicsClient_impl::channel_destroyed() {
  LOG(4, "channel_destroyed()");
  monitoring_stop();
  return 0;
}

void EpicsClient_impl::error_channel_requester() {
  LOG(4, "error_channel_requester()");
}

EpicsClient::EpicsClient(Stream *stream, std::shared_ptr<ForwarderInfo> finfo,
                         std::string epics_channel_provider_type,
                         std::string channel_name)
    : finfo(finfo), stream(stream) {
  impl.reset(new EpicsClient_impl(this));
  if (finfo->teamid != 0) {
    channel_name =
        fmt::format("{}__teamid_{:016x}", channel_name, finfo->teamid);
  }
  CLOG(7, 7, "channel_name: {}", channel_name);
  impl->channel_name = channel_name;
  impl->teamid = finfo->teamid;
  if (impl->init(epics_channel_provider_type) != 0) {
    impl.reset();
    throw std::runtime_error("could not initialize");
  }
}

EpicsClient::~EpicsClient() { CLOG(7, 6, "~EpicsClient"); }

int EpicsClient::stop() { return impl->stop(); }

int EpicsClient::emit(std::unique_ptr<FlatBufs::EpicsPVUpdate> up) {
  return stream->emit(std::move(up));
}

void EpicsClient::error_in_epics() { stream->error_in_epics(); }
}
}
}
