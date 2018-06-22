#pragma once

#include "ConversionWorker.h"
#include "Kafka.h"
#include "RangeSet.h"
#include "SchemaRegistry.h"
#include "uri.h"
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
class KafkaOutput;
struct ConversionWorkPacket;

struct ChannelInfo {
  std::string provider_type;
  std::string channel_name;
};

/**
A combination of a converter and a kafka output destination.
*/
class ConversionPath {
public:
  ConversionPath(ConversionPath &&x);
  ConversionPath(std::shared_ptr<Converter>, std::unique_ptr<KafkaOutput>);
  ~ConversionPath();
  int emit(std::shared_ptr<FlatBufs::EpicsPVUpdate> up);
  std::atomic<uint32_t> transit{0};
  nlohmann::json status_json() const;

private:
  std::shared_ptr<Converter> converter;
  std::unique_ptr<KafkaOutput> kafka_output;
};

/**
Represents a stream from an EPICS PV through a Converter into a KafkaOutput.
*/
class Stream {
public:
  explicit Stream(
      ChannelInfo channel_info,
      std::shared_ptr<EpicsClient::EpicsClientInterface> client,
      std::shared_ptr<
          moodycamel::ConcurrentQueue<std::shared_ptr<FlatBufs::EpicsPVUpdate>>>
          ring);
  Stream(Stream &&) = delete;
  ~Stream();
  int converter_add(InstanceSet &kset, std::shared_ptr<Converter> conv,
                    URI uri_kafka_output);
  int32_t fill_conversion_work(
      moodycamel::ConcurrentQueue<std::unique_ptr<ConversionWorkPacket>> &queue,
      uint32_t max, std::function<void(uint64_t)> on_seq_data);
  int stop();
  void error_in_epics();
  int status();
  ChannelInfo const &channel_info() const;
  size_t emit_queue_size();
  nlohmann::json status_json();
  using mutex = std::mutex;
  using ulock = std::unique_lock<mutex>;

private:
  /// Each Epics update is converted by each Converter in the list
  ChannelInfo channel_info_;
  std::vector<std::unique_ptr<ConversionPath>> conversion_paths;
  std::shared_ptr<EpicsClient::EpicsClientInterface> epics_client;
  std::shared_ptr<
      moodycamel::ConcurrentQueue<std::shared_ptr<FlatBufs::EpicsPVUpdate>>>
      emit_queue;
  RangeSet<uint64_t> seq_data_emitted;
};
}
