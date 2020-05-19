// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

#include "Stream.h"
#include "Converter.h"
#include "EpicsClient/EpicsClientMonitor.h"
#include "EpicsPVUpdate.h"
#include "logger.h"
#include <algorithm>

namespace Forwarder {

ConversionPath::ConversionPath(ConversionPath &&x) noexcept
    : converter(std::move(x.converter)),
      kafka_output(std::move(x.kafka_output)) {}

ConversionPath::ConversionPath(std::shared_ptr<Converter> conv,
                               std::unique_ptr<KafkaOutput> ko)
    : converter(std::move(conv)), kafka_output(std::move(ko)) {}

ConversionPath::~ConversionPath() {
  Logger->trace("~ConversionPath");
  while (true) {
    auto x = transit.load();
    if (x == 0)
      break;
    Logger->debug("~ConversionPath  still has transit {}", transit);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }
}

int ConversionPath::emit(std::shared_ptr<FlatBufs::EpicsPVUpdate> up) {
  auto fb = converter->convert(*up);
  if (fb == nullptr) {
    Logger->info("empty converted flat buffer");
    return 1;
  }
  kafka_output->emit(std::move(fb));
  return 0;
}

ConversionPathStatus ConversionPath::GetStatus() const {
  return ConversionPathStatus(converter->schema_name(),
                              kafka_output->Output.brokerAddress(),
                              kafka_output->topicName());
}

std::string ConversionPath::getKafkaTopicName() const {
  return kafka_output->topicName();
}

std::string ConversionPath::getSchemaName() const {
  return converter->schema_name();
}

Stream::Stream(
    ChannelInfo Info, std::shared_ptr<EpicsClient::EpicsClientInterface> Client,
    std::shared_ptr<
        moodycamel::ConcurrentQueue<std::shared_ptr<FlatBufs::EpicsPVUpdate>>>
        Queue)
    : ChannelInfo_(std::move(Info)), Client(std::move(Client)),
      OutputQueue(std::move(Queue)) {}

Stream::~Stream() {
  Logger->trace("~Stream");
  stop();
  Logger->trace("~Stop DONE");
}

int Stream::addConverter(std::unique_ptr<ConversionPath> Path) {
  std::lock_guard<std::mutex> lock(ConversionPathsMutex);

  auto FoundPath = std::find_if(
      ConversionPaths.cbegin(), ConversionPaths.cend(),
      [&Path](const std::unique_ptr<ConversionPath> &TestPath) {
        return Path->getKafkaTopicName() == TestPath->getKafkaTopicName() and
               Path->getSchemaName() == TestPath->getSchemaName();
      });
  if (FoundPath == ConversionPaths.end()) {
    ConversionPaths.push_back(std::move(Path));
    return 0;
  }
  Logger->info("Stream with channel name: {}  KafkaTopicName: {}  "
               "SchemaName: {} already exists.",
               ChannelInfo_.channel_name, Path->getKafkaTopicName(),
               Path->getSchemaName());
  return 1;
}

void Stream::setEpicsError() { Client->errorInEpics(); }

uint32_t Stream::fillConversionQueue(
    moodycamel::ConcurrentQueue<std::unique_ptr<ConversionWorkPacket>> &Queue,
    uint32_t max) {
  uint32_t NumDequeued = 0;
  uint32_t NumQueued = 0;
  auto BufferSize = OutputQueue->size_approx();
  auto ConversionPathSize = ConversionPaths.size();
  std::vector<ConversionWorkPacket *> cwp_last(ConversionPathSize);

  // Add to queue if data still available and queue has enough "space" for all
  // conversion paths for a single update.
  while (NumDequeued < BufferSize && max - NumQueued >= ConversionPathSize) {
    std::shared_ptr<FlatBufs::EpicsPVUpdate> EpicsUpdate;
    auto found = OutputQueue->try_dequeue(EpicsUpdate);
    if (!found) {
      Logger->info("Conversion worker buffer is empty");
      break;
    }
    NumDequeued += 1;
    if (!EpicsUpdate) {
      Logger->info("Empty EPICS PV update");
      continue;
    }
    size_t ConversionPathID = 0;
    for (auto &ConversionPath : ConversionPaths) {
      auto ConversionPacket = std::make_unique<ConversionWorkPacket>();
      cwp_last[ConversionPathID] = ConversionPacket.get();
      ConversionPacket->Path = ConversionPath.get();
      ConversionPacket->Update = EpicsUpdate;
      bool QueuedSuccessful = Queue.enqueue(std::move(ConversionPacket));
      if (!QueuedSuccessful) {
        Logger->info("Conversion work queue is full");
        break;
      }
      ConversionPathID += 1;
      NumQueued += 1;
    }
  }
  if (NumQueued > 0) {
    for (uint32_t i1 = 0; i1 < ConversionPathSize; ++i1) {
      cwp_last[i1]->stream = this;
      ConversionPaths[i1]->transit++;
    }
  }
  return NumQueued;
}

int Stream::stop() {
  if (Client != nullptr) {
    Client->stop();
  }
  return 0;
}

int Stream::status() { return Client->status(); }

ChannelInfo const &Stream::getChannelInfo() const { return ChannelInfo_; }

size_t Stream::getQueueSize() { return OutputQueue->size_approx(); }

StreamStatus Stream::getStatus() {
  StreamStatus CurrentStatus(ChannelInfo_.channel_name,
                             Client->getConnectionState());

  std::transform(
      ConversionPaths.begin(), ConversionPaths.end(),
      std::back_inserter(CurrentStatus.Converters),
      [](std::unique_ptr<ConversionPath> &Path) { return Path->GetStatus(); });

  return CurrentStatus;
}

std::shared_ptr<EpicsClient::EpicsClientInterface> Stream::getEpicsClient() {
  return Client;
}
} // namespace Forwarder
