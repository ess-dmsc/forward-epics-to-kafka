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
  LOG(Sev::Debug, "~ConversionPath");
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
    ChannelInfo Info, std::shared_ptr<EpicsClient::EpicsClientInterface> Client,
    std::shared_ptr<
        moodycamel::ConcurrentQueue<std::shared_ptr<FlatBufs::EpicsPVUpdate>>>
        Queue)
    : ChannelInfo_(std::move(Info)), Client(std::move(Client)),
      OutputQueue(std::move(Queue)) {}

Stream::~Stream() {
  CLOG(7, 2, "~Stream");
  stop();
  CLOG(7, 2, "~Stop DONE");
  LOG(Sev::Info, "SeqDataEmitted: {}", SeqDataEmitted.to_string());
}

int Stream::addConverter(std::unique_ptr<ConversionPath> Path) {
  std::unique_lock<std::mutex> lock(ConversionPathsMutex);

  for (auto const &ConversionPath : ConversionPaths) {
    if (ConversionPath->getKafkaTopicName() == Path->getKafkaTopicName() &&
        ConversionPath->getSchemaName() == Path->getSchemaName()) {
      LOG(Sev::Notice,
          "Stream with channel name: {}  KafkaTopicName: {}  SchemaName: {} "
          " already exists.",
          ChannelInfo_.channel_name, ConversionPath->getKafkaTopicName(),
          ConversionPath->getSchemaName());
      return 1;
    }
  }
  ConversionPaths.push_back(std::move(Path));
  return 0;
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
      CLOG(6, 1, "Conversion worker buffer is empty");
      break;
    }
    NumDequeued += 1;
    if (!EpicsUpdate) {
      LOG(Sev::Info, "Empty EPICS PV update");
      continue;
    }
    size_t ConversionPathID = 0;
    for (auto &ConversionPath : ConversionPaths) {
      auto ConversionPacket = ::make_unique<ConversionWorkPacket>();
      cwp_last[ConversionPathID] = ConversionPacket.get();
      ConversionPacket->cp = ConversionPath.get();
      ConversionPacket->up = EpicsUpdate;
      bool QueuedSuccessful = Queue.enqueue(std::move(ConversionPacket));
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

nlohmann::json Stream::getStatusJson() {
  using nlohmann::json;
  auto Document = json::object();
  auto const &ChannelInfo = getChannelInfo();
  Document["channel_name"] = ChannelInfo.channel_name;
  Document["getQueueSize"] = getQueueSize();
  {
    std::unique_lock<std::mutex> lock(SeqDataEmitted.mx);
    auto const &Set = SeqDataEmitted.set;
    auto Last = Set.rbegin();
    if (Last != Set.rend()) {
      Document["emitted_max"] = Last->b;
    }
  }
  auto Converters = json::array();
  for (auto const &Converter : ConversionPaths) {
    Converters.push_back(Converter->status_json());
  }
  Document["converters"] = Converters;
  return Document;
}

std::shared_ptr<EpicsClient::EpicsClientInterface> Stream::getEpicsClient() {
  return Client;
}
} // namespace Forwarder
