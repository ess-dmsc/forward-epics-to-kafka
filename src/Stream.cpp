#include "Stream.h"
#include "Converter.h"
#include "EpicsClient/EpicsClientMonitor.h"
#include "EpicsPVUpdate.h"
#include "helper.h"
#include "logger.h"

namespace Forwarder {

ConversionPath::ConversionPath(ConversionPath &&x) noexcept
    : converter(std::move(x.converter)),
      kafka_output(std::move(x.kafka_output)) {}

ConversionPath::ConversionPath(std::shared_ptr<Converter> conv,
                               std::unique_ptr<KafkaOutput> ko)
    : converter(conv), kafka_output(std::move(ko)) {}

ConversionPath::~ConversionPath() {
  LOG(7, "~ConversionPath");
  while (true) {
    auto x = transit.load();
    if (x == 0)
      break;
    CLOG(7, 1, "~ConversionPath  still has transit {}", transit);
    sleep_ms(1000);
  }
}

int ConversionPath::emit(std::shared_ptr<FlatBufs::EpicsPVUpdate> up) {
  auto fb = converter->convert(*up);
  if (fb == nullptr) {
    CLOG(6, 1, "empty converted flat buffer");
    return 1;
  }
  kafka_output->emit(std::move(fb));
  return 0;
}

nlohmann::json ConversionPath::status_json() const {
  using nlohmann::json;
  auto Document = json::object();
  Document["schema"] = converter->schema_name();
  Document["broker"] =
      kafka_output->pt.Producer_->ProducerBrokerSettings.Address;
  Document["topic"] = kafka_output->topic_name();
  return Document;
}

std::string ConversionPath::getKafkaTopicName() const {
  return kafka_output->topic_name();
}

std::string ConversionPath::getSchemaName() const {
  return converter->schema_name();
}

Stream::Stream(
    ChannelInfo channel_info,
    std::shared_ptr<EpicsClient::EpicsClientInterface> client,
    std::shared_ptr<
        moodycamel::ConcurrentQueue<std::shared_ptr<FlatBufs::EpicsPVUpdate>>>
        ring)
    : channel_info_(channel_info), epics_client(std::move(client)),
      emit_queue(ring) {}

Stream::~Stream() {
  CLOG(7, 2, "~Stream");
  stop();
  CLOG(7, 2, "~Stop DONE");
  LOG(6, "seq_data_emitted: {}", seq_data_emitted.to_string());
}

int Stream::converter_add(std::unique_ptr<ConversionPath> cp) {
  std::unique_lock<std::mutex> lock(conversion_paths_mx);

  for (auto const &ConversionPath : conversion_paths) {
    if (ConversionPath->getKafkaTopicName() == cp->getKafkaTopicName() &&
        ConversionPath->getSchemaName() == cp->getSchemaName()) {
      LOG(5,
          "Stream with channel name: {}  KafkaTopicName: {}  SchemaName: {} "
          " already exists.",
          channel_info_.channel_name, ConversionPath->getKafkaTopicName(),
          ConversionPath->getSchemaName());
      return 1;
    }
  }
  conversion_paths.push_back(std::move(cp));
  return 0;
}

void Stream::error_in_epics() { epics_client->errorInEpics(); }

int32_t Stream::fill_conversion_work(
    moodycamel::ConcurrentQueue<std::unique_ptr<ConversionWorkPacket>> &q2,
    uint32_t max) {
  uint32_t NumDequeued = 0;
  uint32_t NumQueued = 0;
  auto BufferSize = emit_queue->size_approx();
  auto ConversionPathSize = conversion_paths.size();
  std::vector<ConversionWorkPacket *> cwp_last(ConversionPathSize);

  // Add to queue if data still available and queue has enough "space" for all
  // conversion paths for a single update.
  while (NumDequeued < BufferSize && max - NumQueued >= ConversionPathSize) {
    std::shared_ptr<FlatBufs::EpicsPVUpdate> EpicsUpdate;
    auto found = emit_queue->try_dequeue(EpicsUpdate);
    if (!found) {
      CLOG(6, 1, "Conversion worker buffer is empty");
      break;
    }
    NumDequeued += 1;
    if (!EpicsUpdate) {
      LOG(6, "Empty EPICS PV update");
      continue;
    }
    size_t ConversionPathID = 0;
    for (auto &ConversionPath : conversion_paths) {
      auto ConversionPacket = ::make_unique<ConversionWorkPacket>();
      cwp_last[ConversionPathID] = ConversionPacket.get();
      ConversionPacket->cp = ConversionPath.get();
      ConversionPacket->up = EpicsUpdate;
      bool QueuedSuccessful = q2.enqueue(std::move(ConversionPacket));
      if (!QueuedSuccessful) {
        CLOG(6, 1, "Conversion work queue is full");
        break;
      }
      ConversionPathID += 1;
      NumQueued += 1;
    }
  }
  if (NumQueued > 0) {
    for (uint32_t i1 = 0; i1 < ConversionPathSize; ++i1) {
      cwp_last[i1]->stream = this;
      conversion_paths[i1]->transit++;
    }
  }
  return NumQueued;
}

int Stream::stop() {
  if (epics_client != nullptr) {
    epics_client->stop();
  }
  return 0;
}

int Stream::status() { return epics_client->status(); }

ChannelInfo const &Stream::channel_info() const { return channel_info_; }

size_t Stream::emit_queue_size() { return emit_queue->size_approx(); }

nlohmann::json Stream::status_json() {
  using nlohmann::json;
  auto Document = json::object();
  auto const &ChannelInfo = channel_info();
  Document["channel_name"] = ChannelInfo.channel_name;
  Document["emit_queue_size"] = emit_queue_size();
  {
    std::unique_lock<std::mutex> lock(seq_data_emitted.mx);
    auto const &Set = seq_data_emitted.set;
    auto Last = Set.rbegin();
    if (Last != Set.rend()) {
      Document["emitted_max"] = Last->b;
    }
  }
  auto Converters = json::array();
  for (auto const &Converter : conversion_paths) {
    Converters.push_back(Converter->status_json());
  }
  Document["converters"] = Converters;
  return Document;
}

std::shared_ptr<EpicsClient::EpicsClientInterface> Stream::getEpicsClient() {
  return epics_client;
}
} // namespace Forwarder
