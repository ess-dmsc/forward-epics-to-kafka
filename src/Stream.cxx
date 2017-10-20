#include "Stream.h"
#include "Converter.h"
#include "EpicsClient.h"
#include "KafkaOutput.h"
#include "epics-pvstr.h"
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

rapidjson::Document ConversionPath::status_json() const {
  using namespace rapidjson;
  Document jd;
  auto &a = jd.GetAllocator();
  jd.SetObject();
  jd.AddMember("schema", Value(converter->schema_name().data(), a), a);
  jd.AddMember("broker",
               Value(kafka_output->pt.producer->opt.address.data(), a), a);
  jd.AddMember("topic", Value(kafka_output->topic_name().data(), a), a);
  return jd;
}

Stream::Stream(std::shared_ptr<ForwarderInfo> finfo, ChannelInfo channel_info)
    : finfo(finfo), channel_info_(channel_info) {
  emit_queue.formatter = _fmt;
  try {
    auto x = new EpicsClient::EpicsClient(
        this, finfo, channel_info.provider_type, channel_info.channel_name);
    epics_client.reset(x);
  } catch (std::runtime_error &e) {
    throw std::runtime_error("can not construct Stream");
  }
}

Stream::Stream(ChannelInfo channel_info) : channel_info_(channel_info){};

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
  // CLOG(9, 7, "Stream::emit");
  if (!up) {
    CLOG(6, 1, "empty update?");
    // should never happen, ignore
    return 0;
  }
  auto seq_data = up->seq_data;
  if (true) {
    for (int i1 = 0; i1 < 256; ++i1) {
      auto x = emit_queue.push(up);
      if (x == 0) {
        break;
      }
      {
        // CLOG(9, 1, "buffer full {} times", i1);
        emit_queue.push_enlarge(up);
        break;
      }
    }
    if (up) {
      // here we are, saying goodbye to a good buffer
      // LOG(4, "loosing buffer {}", up->seq_data);
      up.reset();
      return 1;
    }
    // auto s1 = emit_queue.to_vec();
    // LOG(9, "Queue {}\n{}", channel_info.channel_name, s1.data());
  } else {
    // Emit directly
    // LOG(9, "Stream::emit  convs: {}", conversion_paths.size());
    for (auto &cp : conversion_paths) {
      cp->emit(std::move(up));
    }
  }
  if (false) {
    seq_data_emitted.insert(seq_data);
  }
  return 0;
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
  // LOG(8, "Stream::fill_conversion_work {}  {}  {}", n1, n2, n3);
  while (n0 < n2 && n3 - n1 >= ncp) {
    // LOG(8, "Stream::fill_conversion_work  loop   {}  {}  {}", n1, n2, n3);
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

ChannelInfo const &Stream::channel_info() { return channel_info_; }

size_t Stream::emit_queue_size() { return emit_queue.size(); }

rapidjson::Document Stream::status_json() {
  using namespace rapidjson;
  Document jd;
  auto &a = jd.GetAllocator();
  jd.SetObject();
  auto cthis = (Stream *)this;
  auto &ci = cthis->channel_info();
  jd.AddMember("channel_name", Value(ci.channel_name.data(), a), a);
  jd.AddMember("emit_queue_size", Value().SetUint64(cthis->emit_queue_size()),
               a);
  {
    std::unique_lock<std::mutex> lock(seq_data_emitted.mx);
    auto it = seq_data_emitted.set.rbegin();
    if (it != seq_data_emitted.set.rend()) {
      jd.AddMember("emitted_max", Value().SetUint64(it->b), a);
    }
  }
  Value cps;
  cps.SetArray();
  for (auto &cp : conversion_paths) {
    cps.PushBack(Value().CopyFrom(cp->status_json(), a), a);
  }
  jd.AddMember("converters", cps, a);
  return jd;
}
}
}
