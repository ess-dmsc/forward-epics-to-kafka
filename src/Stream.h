// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

#pragma once

#include "ConversionWorker.h"
#include "EpicsClient/EpicsClientInterface.h"
#include "Kafka.h"
#include "KafkaOutput.h"
#include "RangeSet.h"
#include "SchemaRegistry.h"
#include "StreamStatus.h"
#include "URI.h"
#include <array>
#include <atomic>
#include <concurrentqueue/concurrentqueue.h>
#include <memory>
#include <string>
#include <vector>

namespace Forwarder {

class Converter;
struct ConversionWorkPacket;

struct ChannelInfo {
  std::string provider_type;
  std::string channel_name;
};

/// A combination of a converter and a kafka output destination.
class ConversionPath {
public:
  ConversionPath(ConversionPath &&x) noexcept;
  ConversionPath(std::shared_ptr<Converter>, std::unique_ptr<KafkaOutput>);
  virtual ~ConversionPath();
  int emit(std::shared_ptr<FlatBufs::EpicsPVUpdate> up);
  std::atomic<uint32_t> transit{0};
  ConversionPathStatus GetStatus() const;
  virtual std::string getKafkaTopicName() const;
  virtual std::string getSchemaName() const;

private:
  std::shared_ptr<Converter> converter;
  std::unique_ptr<KafkaOutput> kafka_output;
  SharedLogger Logger = getLogger();
};

/// Represents a stream from an EPICS PV through a Converter into a KafkaOutput.
class Stream {
public:
  Stream(
      ChannelInfo Info,
      std::shared_ptr<EpicsClient::EpicsClientInterface> Client,
      std::shared_ptr<
          moodycamel::ConcurrentQueue<std::shared_ptr<FlatBufs::EpicsPVUpdate>>>
          Queue);
  Stream(Stream &&) = delete;
  ~Stream();
  int addConverter(std::unique_ptr<ConversionPath> Path);
  uint32_t fillConversionQueue(
      moodycamel::ConcurrentQueue<std::unique_ptr<ConversionWorkPacket>> &Queue,
      uint32_t max);
  int stop();
  void setEpicsError();
  int status();
  ChannelInfo const &getChannelInfo() const;
  std::shared_ptr<EpicsClient::EpicsClientInterface> getEpicsClient();
  size_t getQueueSize();
  StreamStatus getStatus();

private:
  /// Each Epics update is converted by each Converter in the list
  ChannelInfo ChannelInfo_;
  std::vector<std::unique_ptr<ConversionPath>> ConversionPaths;
  std::shared_ptr<EpicsClient::EpicsClientInterface> Client;
  std::shared_ptr<
      moodycamel::ConcurrentQueue<std::shared_ptr<FlatBufs::EpicsPVUpdate>>>
      OutputQueue;

  /// We want to be able to add conversion paths after forwarding is running.
  /// Therefore, we need mutually exclusive access to 'conversion_paths'.
  std::mutex ConversionPathsMutex;
  SharedLogger Logger = getLogger();
};
} // namespace Forwarder
