#include "EpicsClientMonitor.h"
#include "ChannelRequester.h"
#include "EpicsClientMonitorImpl.h"
#include "EpicsPVUpdate.h"
#include "flatbuffers/flatbuffers.h"
#include "logger.h"
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

namespace Forwarder {
namespace EpicsClient {

namespace ep00 {
#include <ep00_epics_connection_info_generated.h>
}

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

std::string EpicsClientMonitor::getConnectionState() {
  return toString(ConnectionState);
}

void EpicsClientMonitor::handleConnectionStateChange(
    ChannelConnectionState ConnectionState) {
  LOG(Sev::Info, "EpicsClientMonitor::handleConnectionStateChange  {}",
      toString(ConnectionState));
  this->ConnectionState = ConnectionState;
  if (ConnectionState == ChannelConnectionState::CONNECTED) {
    Impl->monitoringStart();
  } else {
    Impl->monitoringStop();
  }
  if (ConnectionStatusProducer != nullptr) {
    flatbuffers::FlatBufferBuilder Builder;
    auto PVName = Builder.CreateString(Impl->channel_name);
    auto ServiceID = Builder.CreateString(this->ServiceID);
    auto Timestamp = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count());
    auto InfoBuffer = ep00::EpicsConnectionInfoBuilder(Builder);
    InfoBuffer.add_timestamp(Timestamp);
    InfoBuffer.add_source_name(PVName);
    InfoBuffer.add_service_id(ServiceID);
    if (ConnectionState == ChannelConnectionState::NEVER_CONNECTED) {
      InfoBuffer.add_type(ep00::EventType::NEVER_CONNECTED);
    } else if (ConnectionState == ChannelConnectionState::CONNECTED) {
      InfoBuffer.add_type(ep00::EventType::CONNECTED);
    } else if (ConnectionState == ChannelConnectionState::DISCONNECTED) {
      InfoBuffer.add_type(ep00::EventType::DISCONNECTED);
    } else if (ConnectionState == ChannelConnectionState::DESTROYED) {
      InfoBuffer.add_type(ep00::EventType::DESTROYED);
    }
    ep00::FinishEpicsConnectionInfoBuffer(Builder, InfoBuffer.Finish());
    ConnectionStatusProducer->produceAndSetKey(Builder.GetBufferPointer(),
                                      Builder.GetSize(), Impl->channel_name);
  }
}

void EpicsClientMonitor::handleChannelRequesterError(
    std::string const &Message) {
  LOG(Sev::Warning, "EpicsClientMonitor received: {}", Message);
}

void EpicsClientMonitor::setServiceID(std::string ServiceID) {
  this->ServiceID = ServiceID;
}

} // namespace EpicsClient
} // namespace Forwarder
