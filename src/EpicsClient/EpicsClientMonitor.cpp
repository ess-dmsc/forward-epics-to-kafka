// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

#include "EpicsClientMonitor.h"
#include "../EpicsPVUpdate.h"
#include "../RangeSet.h"
#include "../logger.h"
#include "ChannelRequester.h"
#include "EpicsClientMonitorImpl.h"
#include <atomic>
#include <flatbuffers/flatbuffers.h>
#include <memory>
#include <mutex>
#include <pv/pvAccess.h>
#include <utility>
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
        Ring)
    : EmitQueue(std::move(Ring)) {
  Impl.reset(new EpicsClientMonitorImpl(this));
  Logger->debug("channel_name: {}", ChannelInfo.channel_name);
  Impl->channel_name = ChannelInfo.channel_name;
  if (Impl->init(ChannelInfo.provider_type) != 0) {
    Impl.reset();
    throw std::runtime_error("could not initialize");
  }
}

EpicsClientMonitor::~EpicsClientMonitor() {
  Logger->trace("EpicsClientMonitorMonitor");
}

int EpicsClientMonitor::stop() { return Impl->stop(); }

void EpicsClientMonitor::emit(std::shared_ptr<FlatBufs::EpicsPVUpdate> Update) {
  std::lock_guard<std::mutex> lock(CachedUpdateMutex);
  CachedUpdate = Update;
  emitWithoutCaching(Update);
}

void EpicsClientMonitor::errorInEpics() { status_ = -1; }

void EpicsClientMonitor::emitCachedValue() {
  std::lock_guard<std::mutex> lock(CachedUpdateMutex);
  if (CachedUpdate != nullptr) {
    emitWithoutCaching(CachedUpdate);
  }
}
void EpicsClientMonitor::emitWithoutCaching(
    std::shared_ptr<FlatBufs::EpicsPVUpdate> Update) {
  assert(Update != nullptr);
  EmitQueue->enqueue(Update);
}

std::string EpicsClientMonitor::getConnectionState() {
  return toString(ConnectionState);
}

void EpicsClientMonitor::handleConnectionStateChange(
    ChannelConnectionState NewConnectionState) {
  Logger->info("EpicsClientMonitor::handleConnectionStateChange  {}",
               toString(NewConnectionState));
  this->ConnectionState = NewConnectionState;
  if (NewConnectionState == ChannelConnectionState::CONNECTED) {
    Impl->monitoringStart();
  } else {
    Impl->monitoringStop();
  }
  if (ConnectionStatusProducer != nullptr) {
    flatbuffers::FlatBufferBuilder Builder;
    auto PVName = Builder.CreateString(Impl->channel_name);
    auto ServiceIDStr = Builder.CreateString(this->ServiceID);
    auto Timestamp = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count());
    auto InfoBuffer = ep00::EpicsConnectionInfoBuilder(Builder);
    InfoBuffer.add_timestamp(Timestamp);
    InfoBuffer.add_source_name(PVName);
    InfoBuffer.add_service_id(ServiceIDStr);
    if (NewConnectionState == ChannelConnectionState::NEVER_CONNECTED) {
      InfoBuffer.add_type(ep00::EventType::NEVER_CONNECTED);
    } else if (NewConnectionState == ChannelConnectionState::CONNECTED) {
      InfoBuffer.add_type(ep00::EventType::CONNECTED);
    } else if (NewConnectionState == ChannelConnectionState::DISCONNECTED) {
      InfoBuffer.add_type(ep00::EventType::DISCONNECTED);
    } else if (NewConnectionState == ChannelConnectionState::DESTROYED) {
      InfoBuffer.add_type(ep00::EventType::DESTROYED);
    }
    ep00::FinishEpicsConnectionInfoBuffer(Builder, InfoBuffer.Finish());
    ConnectionStatusProducer->produceAndSetKey(
        Builder.GetBufferPointer(), Builder.GetSize(), Impl->channel_name);
  }
}

void EpicsClientMonitor::handleChannelRequesterError(
    std::string const &Message) {
  Logger->warn("EpicsClientMonitor received: {}", Message);
}

void EpicsClientMonitor::setServiceID(const std::string &NewServiceID) {
  this->ServiceID = NewServiceID;
}

void EpicsClientMonitor::setProducer(
    std::unique_ptr<KafkaW::ProducerTopic> Producer) {
  this->ConnectionStatusProducer = std::move(Producer);
}
} // namespace EpicsClient
} // namespace Forwarder
