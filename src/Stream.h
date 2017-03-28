#pragma once

#include <memory>
#include <atomic>
#include <array>
#include <vector>
#include <string>
#include "uri.h"
#include "SchemaRegistry.h"
#include "ConversionWorker.h"
#include "Kafka.h"
#include "Ring.h"

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
ConversionPath(ConversionPath && x);
ConversionPath(std::shared_ptr<Converter>, std::unique_ptr<KafkaOutput>);
~ConversionPath();
int emit(std::unique_ptr<FlatBufs::EpicsPVUpdate> up);
std::atomic<uint32_t> transit {0};
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
int converter_add(Kafka::InstanceSet & kset, std::shared_ptr<Converter> conv, uri::URI uri_kafka_output);
int emit(std::unique_ptr<FlatBufs::EpicsPVUpdate> up);
int32_t fill_conversion_work(Ring<std::unique_ptr<ConversionWorkPacket>> & queue, uint32_t max);
int stop();
void teamid(uint64_t);
void error_in_epics();
int status();
ChannelInfo const & channel_info();
using mutex = std::mutex;
using ulock = std::unique_lock<mutex>;
private:
/// Each Epics update is converted by each Converter in the list
//std::vector<std::shared_ptr<Converter>> converters;
ChannelInfo channel_info_;
std::vector<std::unique_ptr<ConversionPath>> conversion_paths;
uint64_t teamid_;
std::unique_ptr<EpicsClient::EpicsClient> epics_client;
Ring<std::unique_ptr<FlatBufs::EpicsPVUpdate>> emit_queue;
std::atomic<int> status_ {0};
};

}
}
