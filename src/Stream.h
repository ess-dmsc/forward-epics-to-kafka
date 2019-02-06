#pragma once

#include "ConversionWorker.h"
#include "Kafka.h"
#include "KafkaOutput.h"
#include "RangeSet.h"
#include "SchemaRegistry.h"
#include "URI.h"
#include <EpicsClient/EpicsClientInterface.h>
#include <array>
#include <atomic>
#include <concurrentqueue/concurrentqueue.h>
#include <memory>
#include <nlohmann/json.hpp>
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
  nlohmann::json status_json() const;
  virtual std::string getKafkaTopicName() const;
  virtual std::string getSchemaName() const;

private:
  std::shared_ptr<Converter> converter;
  std::unique_ptr<KafkaOutput> kafka_output;
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
  nlohmann::json getStatusJson();

private:
  /// Each Epics update is converted by each Converter in the list
  ChannelInfo ChannelInfo_;
  std::vector<std::unique_ptr<ConversionPath>> ConversionPaths;
  std::shared_ptr<EpicsClient::EpicsClientInterface> Client;
  std::shared_ptr<
      moodycamel::ConcurrentQueue<std::shared_ptr<FlatBufs::EpicsPVUpdate>>>
      OutputQueue;
  RangeSet<uint64_t> SeqDataEmitted;

  /// We want to be able to add conversion paths after forwarding is running.
  /// Therefore, we need mutually exclusive access to 'conversion_paths'.
  std::mutex ConversionPathsMutex;
};
} // namespace Forwarder
