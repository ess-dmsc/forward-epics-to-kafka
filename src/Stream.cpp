#include "Stream.h"
#include "Converter.h"
#include "EpicsClient/EpicsClientMonitor.h"
#include "KafkaOutput.h"
#include "epics-to-fb.h"
#include "helper.h"
#include "logger.h"

namespace BrightnESS {
namespace ForwardEpicsToKafka {

ConversionPath::ConversionPath(ConversionPath &&x)
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

int ConversionPath::emit(std::unique_ptr<FlatBufs::EpicsPVUpdate> up) {
  auto fb = converter->convert(*up);
  if (fb == nullptr) {
    CLOG(6, 1, "empty converted flat buffer");
    return 1;
  }
  kafka_output->emit(std::move(fb));
  return 0;
}

static uint16_t _fmt(std::unique_ptr<FlatBufs::EpicsPVUpdate> &x) {
  return (uint16_t)(((uint64_t)x.get()) >> 0);
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

Stream::Stream(ChannelInfo channel_info) : channel_info_(channel_info) {
  emit_queue.formatter = _fmt;
  try {
    auto x = new EpicsClient::EpicsClientMonitor(
        this, channel_info.provider_type, channel_info.channel_name);
    epics_client.reset(x);
  } catch (std::runtime_error &e) {
    throw std::runtime_error("can not construct Stream");
  }
}

Stream::~Stream() {
  CLOG(7, 2, "~Stream");
  stop();
  CLOG(7, 2, "~Stop DONE");
  LOG(6, "seq_data_emitted: {}", seq_data_emitted.to_string());
}

int Stream::converter_add(Kafka::InstanceSet &kset, Converter::sptr conv,
                          uri::URI uri_kafka_output) {
  auto pt = kset.producer_topic(uri_kafka_output);
  std::unique_ptr<ConversionPath> cp(new ConversionPath(
      {std::move(conv)},
      std::unique_ptr<KafkaOutput>(new KafkaOutput(std::move(pt)))));
  conversion_paths.push_back(std::move(cp));
  return 0;
}

int Stream::emit(std::unique_ptr<FlatBufs::EpicsPVUpdate> up) {
  if (!up) {
    CLOG(6, 1, "empty update?");
    // should never happen, ignore
    return 0;
  }

  emit_queue.push_enlarge(up);

  // here we are, saying goodbye to a good buffer
  up.reset();
  return 1;
}

int32_t
Stream::fill_conversion_work(Ring<std::unique_ptr<ConversionWorkPacket>> &q2,
                             uint32_t max,
                             std::function<void(uint64_t)> on_seq_data) {
  auto &q1 = emit_queue;
  uint32_t n0 = 0;
  uint32_t n1 = 0;
  ulock l1(q1.mx);
  ulock l2(q2.mx);
  uint32_t n2 = q1.size_unsafe();
  uint32_t n3 = (std::min)(max, q2.capacity_unsafe() - q2.size_unsafe());
  uint32_t ncp = conversion_paths.size();
  std::vector<ConversionWorkPacket *> cwp_last(conversion_paths.size());
  while (n0 < n2 && n3 - n1 >= ncp) {
    auto e = q1.pop_unsafe();
    n0 += 1;
    if (e.first != 0) {
      CLOG(6, 1, "empty? should not happen");
      break;
    }
    auto &up = e.second;
    if (!up) {
      LOG(6, "empty epics update");
      continue;
    }
    size_t cpid = 0;
    uint32_t ncp = conversion_paths.size();
    on_seq_data(e.second->seq_data);
    for (auto &cp : conversion_paths) {
      auto p = std::unique_ptr<ConversionWorkPacket>(new ConversionWorkPacket);
      cwp_last[cpid] = p.get();
      p->cp = cp.get();
      if (ncp == 1) {
        // more common case
        p->up = std::move(e.second);
      } else {
        p->up = std::unique_ptr<FlatBufs::EpicsPVUpdate>(
            new FlatBufs::EpicsPVUpdate(*e.second));
      }
      auto x = q2.push_unsafe(p);
      if (x != 0) {
        CLOG(6, 1, "full? should not happen");
        break;
      }
      cpid += 1;
      n1 += 1;
    }
  }
  if (n1 > 0) {
    for (uint32_t i1 = 0; i1 < cwp_last.size(); ++i1) {
      cwp_last[i1]->stream = this;
      conversion_paths[i1]->transit++;
    }
  }
  return n1;
}

int Stream::stop() {
  if (epics_client != nullptr) {
    epics_client->stop();
  }
  return 0;
}

void Stream::error_in_epics() { status_ = -1; }

int Stream::status() { return status_; }

ChannelInfo const &Stream::channel_info() const { return channel_info_; }

size_t Stream::emit_queue_size() { return emit_queue.size(); }

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
}
}
