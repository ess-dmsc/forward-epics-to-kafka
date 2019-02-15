#include "EpicsClientMonitor.h"
#include "ChannelRequester.h"
#include "EpicsClientMonitorImpl.h"
#include <atomic>
#include <memory>
#include <mutex>
#include <utility>
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

using epics::pvAccess::Channel;
using epics::pvData::PVStructure;

EpicsClientMonitor::EpicsClientMonitor(
    ChannelInfo &ChannelInfo,
    std::shared_ptr<
        moodycamel::ConcurrentQueue<std::shared_ptr<FlatBufs::EpicsPVUpdate>>>
        EmitQueue)
    : EmitQueue(std::move(EmitQueue)) {
  Impl.reset(new EpicsClientMonitorImpl(this));
  LOG(Sev::Debug, "channel_name: {}", ChannelInfo.channel_name);
  Impl->channel_name = ChannelInfo.channel_name;
  if (Impl->init(ChannelInfo.provider_type) != 0) {
    Impl.reset();
    throw std::runtime_error("could not initialize");
  }
}

EpicsClientMonitor::~EpicsClientMonitor() {
  LOG(Sev::Debug, "EpicsClientMonitorMonitor");
}

int EpicsClientMonitor::stop() { return Impl->stop(); }

int EpicsClientMonitor::emit(std::shared_ptr<FlatBufs::EpicsPVUpdate> Update) {
  CachedUpdate = Update;
  return emitWithoutCaching(Update);
}

void EpicsClientMonitor::errorInEpics() { status_ = -1; }

void EpicsClientMonitor::emitCachedValue() {
  if (CachedUpdate != nullptr) {
    emitWithoutCaching(CachedUpdate);
  }
}
int EpicsClientMonitor::emitWithoutCaching(
    std::shared_ptr<FlatBufs::EpicsPVUpdate> Update) {
  if (!Update) {
    LOG(Sev::Info, "empty update?");
    // should never happen, ignore
    return 1;
  }
  EmitQueue->enqueue(Update);
  return 0;
}

std::string EpicsClientMonitor::getConnectionStatus() {
  return ConnectionStatus;
}

void EpicsClientMonitor::handleConnectionStateChange(
    std::string const &ConnectionState) {
  LOG(Sev::Info, "EpicsClientMonitor::handleConnectionStateChange  {}",
      ConnectionState);
  ConnectionStatus = ConnectionState;
  if (ConnectionStatus == "CONNECTED") {
    Impl->monitoringStart();
  } else {
    Impl->monitoringStop();
  }
  if (ConnectionStatusProducer != nullptr) {
    auto Message = fmt::format("ConnectionStatus: {}", ConnectionStatus);
    ConnectionStatusProducer->produce((unsigned char *)Message.data(),
                                      Message.size());
  }
}

void EpicsClientMonitor::handleChannelRequesterError(
    std::string const &Message) {
  LOG(Sev::Warning, "EpicsClientMonitor received: {}", Message);
}

} // namespace EpicsClient
} // namespace Forwarder
