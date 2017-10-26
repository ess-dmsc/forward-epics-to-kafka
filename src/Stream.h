#pragma once

#include "ConversionWorker.h"
#include "Kafka.h"
#include "RangeSet.h"
#include "Ring.h"
#include "SchemaRegistry.h"
#include "uri.h"
#include <array>
#include <atomic>
#include <memory>
#include <rapidjson/document.h>
#include <string>
#include <vector>

namespace BrightnESS {
namespace ForwardEpicsToKafka {

class Converter;
class KafkaOutput;
class ForwarderInfo;
struct ConversionWorkPacket;

namespace EpicsClient {
class EpicsClient;
}

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
  int emit(std::unique_ptr<FlatBufs::EpicsPVUpdate> up);
  std::atomic<uint32_t> transit{0};
  rapidjson::Document status_json() const;

private:
  std::shared_ptr<Converter> converter;
  std::unique_ptr<KafkaOutput> kafka_output;
};

/**
Represents a stream from an EPICS PV through a Converter into a KafkaOutput.
*/
class Stream {
public:
  Stream(std::shared_ptr<ForwarderInfo> finfo, ChannelInfo channel_info);
  Stream(Stream &&) = delete;
  ~Stream();
  int converter_add(Kafka::InstanceSet &kset, std::shared_ptr<Converter> conv,
                    uri::URI uri_kafka_output);
  int emit(std::unique_ptr<FlatBufs::EpicsPVUpdate> up);
  int32_t
  fill_conversion_work(Ring<std::unique_ptr<ConversionWorkPacket>> &queue,
                       uint32_t max, std::function<void(uint64_t)> on_seq_data);
  int stop();
  void error_in_epics();
  int status();
  ChannelInfo const &channel_info();
  size_t emit_queue_size();
  rapidjson::Document status_json();
  using mutex = std::mutex;
  using ulock = std::unique_lock<mutex>;

protected:
  // This constructor is to enable unit-testing.
  // Not to be used outside of testing.
  explicit Stream(ChannelInfo channel_info);

private:
  /// Each Epics update is converted by each Converter in the list
  // std::vector<std::shared_ptr<Converter>> converters;
  std::shared_ptr<ForwarderInfo> finfo;
  ChannelInfo channel_info_;
  std::vector<std::unique_ptr<ConversionPath>> conversion_paths;
  std::unique_ptr<EpicsClient::EpicsClient> epics_client;
  Ring<std::unique_ptr<FlatBufs::EpicsPVUpdate>> emit_queue;
  std::atomic<int> status_{0};
  RangeSet<uint64_t> seq_data_emitted;
};
}
}
