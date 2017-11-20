#include "EpicsClient.h"
#include <atomic>
#include <chrono>
#include <mutex>
#include <pv/pvAccess.h>
#include <pv/pvData.h>
#include <sstream>
// For epics::pvAccess::ClientFactory::start()
#include <pv/clientFactory.h>
// EPICS 4 supports access via the channel access protocol as well,
// and we need it because some hardware speaks EPICS base.
#include <pv/caProvider.h>
//#include "fbhelper.h"
//#include "fbschemas.h"
#include "RangeSet.h"
#include "epics-pvstr.h"
#include "epics-to-fb.h"
#include "logger.h"
#ifdef _MSC_VER
#include <iso646.h>
#endif
#include "RangeSet.h"

namespace BrightnESS {
namespace ForwardEpicsToKafka {
namespace EpicsClient {

using epics::pvData::Structure;
using epics::pvData::PVStructure;
using epics::pvData::Field;
using epics::pvData::MessageType;
using epics::pvAccess::Channel;
using std::mutex;
using ulock = std::unique_lock<mutex>;
using urlock = std::unique_lock<std::recursive_mutex>;

#define STRINGIFY2(x) #x
#define STRINGIFY(x) STRINGIFY2(x)

char const *channel_state_name(epics::pvAccess::Channel::ConnectionState x) {
// Channel::ConnectionStateNames[cstate]
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

static std::string
channel_info(epics::pvAccess::Channel::shared_pointer const &channel) {
  std::ostringstream ss;
  channel->printInfo(ss);
  return ss.str();
}

class ChannelRequester : public epics::pvAccess::ChannelRequester {
public:
  ChannelRequester(EpicsClient_impl *epics_client_impl);
  // From class pvData::Requester
  std::string getRequesterName() override;
  void message(std::string const &message, MessageType messageType) override;
  void channelCreated(
      const epics::pvData::Status &status,
      epics::pvAccess::Channel::shared_pointer const &channel) override;
  void channelStateChange(
      epics::pvAccess::Channel::shared_pointer const &channel,
      epics::pvAccess::Channel::ConnectionState connectionState) override;

private:
  epics::pvData::MonitorRequester::shared_pointer monitor_requester;
  epics::pvData::MonitorPtr monitor;
  EpicsClient_impl *epics_client_impl = nullptr;
};

class FwdMonitorRequester : public ::epics::pvData::MonitorRequester {
public:
  FwdMonitorRequester(EpicsClient_impl *epics_client_impl, string channel_name);
  ~FwdMonitorRequester();
  string getRequesterName() override;
  void message(string const &msg,
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

struct EpicsClientFactoryInit {
  EpicsClientFactoryInit();
  ~EpicsClientFactoryInit();
  static std::unique_ptr<EpicsClientFactoryInit> factory_init();
  static std::atomic<int> count;
  static std::mutex mxl;
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
    if (true) {
      ::epics::pvAccess::ClientFactory::start();
    }
    if (true) {
      ::epics::pvAccess::ca::CAClientFactory::start();
    }
  }
}
EpicsClientFactoryInit::~EpicsClientFactoryInit() {
  CLOG(7, 7, "~EpicsClientFactoryInit");
  ulock lock(mxl);
  auto c = --count;
  if (c < 0) {
    throw std::runtime_error("should never happen");
  }
  if (c == 0) {
    CLOG(7, 6, "STOP   Epics factories");
    if (true) {
      ::epics::pvAccess::ClientFactory::stop();
    }
    if (true) {
      ::epics::pvAccess::ca::CAClientFactory::stop();
    }
  }
}

class EpicsClient_impl {
public:
  EpicsClient_impl(EpicsClient *epics_client);
  ~EpicsClient_impl();
  int init(string epics_channel_provider_type);
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
  int state = 0;
  uint64_t teamid = 0;
  uint64_t fwdix = 0;
  string channel_name;
  EpicsClient *epics_client = nullptr;
  std::unique_ptr<EpicsClientFactoryInit> factory_init;
};

ChannelRequester::ChannelRequester(EpicsClient_impl *epics_client_impl)
    : epics_client_impl(epics_client_impl) {}

string ChannelRequester::getRequesterName() { return "ChannelRequester"; }

void ChannelRequester::message(std::string const &message,
                               MessageType messageType) {
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
          channel_info(channel));
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

FwdMonitorRequester::FwdMonitorRequester(EpicsClient_impl *epics_client_impl,
                                         std::string channel_name)
    : channel_name(channel_name), epics_client_impl(epics_client_impl) {
  static std::atomic<uint32_t> __id{0};
  auto id = __id++;
  name = fmt::format("FwdMonitorRequester-{}", id);
  CLOG(7, 6, "FwdMonitorRequester {}", name);
}

FwdMonitorRequester::~FwdMonitorRequester() {
  CLOG(6, 6, "~FwdMonitorRequester");
  CLOG(6, 6, "~FwdMonitorRequester  seq_data_received: {}",
       seq_data_received.to_string());
}

string FwdMonitorRequester::getRequesterName() { return name; }

void FwdMonitorRequester::message(string const &msg,
                                  ::epics::pvData::MessageType msgT) {
  CLOG(7, 7, "FwdMonitorRequester::message: {}:  {}", name, msg.c_str());
}

void FwdMonitorRequester::monitorConnect(
    ::epics::pvData::Status const &status,
    ::epics::pvData::Monitor::shared_pointer const &monitor,
    ::epics::pvData::StructureConstPtr const &structure) {
  if (!status.isSuccess()) {
    // NOTE
    // Docs does not say anything about whether we are responsible for any
    // handling of the monitor if non-null?
    CLOG(3, 2, "monitorConnect is != success for {}", name);
    epics_client_impl->monitor_requester_error(this);
  } else {
    if (status.isOK()) {
      CLOG(7, 7, "success and OK");
      monitor->start();
    } else {
      CLOG(7, 6, "success with warning");
    }
  }
}

void FwdMonitorRequester::monitorEvent(
    ::epics::pvData::MonitorPtr const &monitor) {
  // CLOG(7, 7, "FwdMonitorRequester::monitorEvent");
  std::vector<std::unique_ptr<FlatBufs::EpicsPVUpdate>> ups;
  while (true) {
    auto ele = monitor->poll();
    if (!ele) {
      break;
    }

    uint64_t seq_data = 0;
    if (false) {
      if (auto x = ele->pvStructurePtr
                       ->getSubField<epics::pvData::PVScalarValue<uint64_t>>(
                           "seq")) {
        seq_data = x->get();
      }
    }

    // CLOG(7, 7, "monitorEvent seq {}", seq);
    static_assert(sizeof(uint64_t) == sizeof(std::chrono::nanoseconds::rep),
                  "Types not compatible");
    uint64_t ts = std::chrono::duration_cast<std::chrono::nanoseconds>(
                      std::chrono::system_clock::now().time_since_epoch())
                      .count();

    // Seems like MonitorElement always returns a Structure type ?
    // The inheritance diagram shows that scalars derive from Field, not from
    // Structure.
    // Does that mean that we never get a scalar here directly??

    auto up_ =
        std::unique_ptr<FlatBufs::EpicsPVUpdate>(new FlatBufs::EpicsPVUpdate);
    auto &up = *up_;
    up.channel = channel_name;
    // up.epics_pvstr->pvstr = ele->pvStructurePtr;
    up.epics_pvstr = epics::pvData::PVStructure::shared_pointer(
        new ::epics::pvData::PVStructure(ele->pvStructurePtr->getStructure()));
    up.epics_pvstr->copyUnchecked(*ele->pvStructurePtr);
    //::epics::pvData::PVStructure s2(ele->pvStructurePtr->getStructure());
    // s2.copy(*ele->pvStructurePtr);
    // up.monitor = (void*)monitor.get();
    // up.ele = (void*)ele_ptr;
    monitor->release(ele);
    up.seq_fwd = seq;
    up.seq_data = seq_data;
    up.ts_epics_monitor = ts;
    up.fwdix = epics_client_impl->fwdix;
    up.teamid = epics_client_impl->teamid;
    ups.push_back(std::move(up_));
    seq += 1;
  }
  for (auto &up : ups) {
    up->epics_pvstr->setImmutable();
    auto seq_data = up->seq_data;
    auto x = epics_client_impl->emit(std::move(up));
    if (x != 0) {
      LOG(5, "error can not push update {}", seq_data);
    } else {
      if (false) {
        seq_data_received.insert(seq_data);
      }
    }
  }
}

void FwdMonitorRequester::unlisten(epics::pvData::MonitorPtr const &monitor) {
  CLOG(7, 1, "FwdMonitorRequester::unlisten  {}", name);
}

EpicsClient_impl::EpicsClient_impl(EpicsClient *epics_client)
    : epics_client(epics_client) {}

int EpicsClient_impl::init(string epics_channel_provider_type) {
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
  string request = "field(value,timeStamp,alarm)";
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
                         string epics_channel_provider_type,
                         string channel_name)
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
