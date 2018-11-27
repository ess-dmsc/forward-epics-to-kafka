#include "Producer.h"
#include "logger.h"
#include "ProducerEventCb.h"

namespace KafkaW {

static std::atomic<int> g_kafka_producer_instance_count;

void Producer::deliveredCallback(rd_kafka_t *RK,
                                 rd_kafka_message_t const *Message,
                                 void *Opaque) {
  auto Self = reinterpret_cast<Producer *>(Opaque);
  if (!Message) {
    LOG(Sev::Error, "IID: {}  ERROR msg should never be null", Self->id);
    ++Self->Stats.produce_cb_fail;
    return;
  }
  if (Message->err) {
    LOG(Sev::Error, "IID: {}  ERROR on delivery, {}, topic {}, {} [{}] {}",
        Self->id, rd_kafka_name(RK), rd_kafka_topic_name(Message->rkt),
        rd_kafka_err2name(Message->err), Message->err,
        rd_kafka_err2str(Message->err));
    if (Message->err == RD_KAFKA_RESP_ERR__MSG_TIMED_OUT) {
    }
    if (auto &Callback = Self->on_delivery_failed) {
      Callback(Message);
    }
    ++Self->Stats.produce_cb_fail;
  } else {
    if (auto &Callback = Self->on_delivery_ok) {
      Callback(Message);
    }

    ++Self->Stats.produce_cb;
  }
}

Producer::~Producer() {
  LOG(Sev::Debug, "~Producer");
  if (RdKafkaPtr) {
    int TimeoutMS = 1;
    uint32_t OutQueueLength = 0;
    while (true) {
      OutQueueLength = rd_kafka_outq_len(RdKafkaPtr);
      if (OutQueueLength == 0) {
        break;
      }
      auto EventsHandled = rd_kafka_poll(RdKafkaPtr, TimeoutMS);
      if (EventsHandled > 0) {
        LOG(Sev::Debug,
            "rd_kafka_poll handled: {}  outq before: {}  timeout: {}",
            EventsHandled, OutQueueLength, TimeoutMS);
      }
      TimeoutMS = TimeoutMS << 1;
      if (TimeoutMS > 8192) {
        break;
      }
    }
    if (OutQueueLength > 0) {
      LOG(Sev::Notice,
          "Kafka out queue still not empty: {}  destroy producer anyway.",
          OutQueueLength);
    }
    LOG(Sev::Debug, "rd_kafka_destroy");
    rd_kafka_destroy(RdKafkaPtr);
    RdKafkaPtr = nullptr;
  }
}

Producer::Producer(BrokerSettings ProducerBrokerSettings)
    : ProducerBrokerSettings(ProducerBrokerSettings) {
  id = g_kafka_producer_instance_count++;

  // librdkafka API sometimes wants to write errors into a buffer:
  std::vector<char> ERRSTR;
  ERRSTR.resize(512);
  std::string errstr;

  ProducerEventCb eventCallback

  RdKafka::Conf Config;
  Config.set("dr_cb", &deliveredCallback, errstr);
  Config.set("event_cb", &eventCallback, errstr);


  rd_kafka_conf_set_opaque(Config, this);
  LOG(Sev::Debug, "Producer opaque: {}", (void *)this);

  ProducerBrokerSettings.apply(Config);

  RdKafkaPtr =
      rd_kafka_new(RD_KAFKA_PRODUCER, Config, ERRSTR.data(), ERRSTR.size());
  if (!RdKafkaPtr) {
    LOG(Sev::Error, "can not create kafka handle: {}", ERRSTR.data());
    throw std::runtime_error("can not create Kafka handle");
  }

  rd_kafka_set_log_level(RdKafkaPtr, 4);

  LOG(Sev::Info, "New Kafka {} with brokers: {}", rd_kafka_name(RdKafkaPtr),
      ProducerBrokerSettings.Address.c_str());
  if (rd_kafka_brokers_add(RdKafkaPtr,
                           ProducerBrokerSettings.Address.c_str()) == 0) {
    LOG(Sev::Error, "could not add brokers");
    throw std::runtime_error("could not add brokers");
  }
}

Producer::Producer(Producer &&x) noexcept {
  using std::swap;
  swap(RdKafkaPtr, x.RdKafkaPtr);
  swap(on_delivery_ok, x.on_delivery_ok);
  swap(on_delivery_failed, x.on_delivery_failed);
  swap(on_error, x.on_error);
  swap(ProducerBrokerSettings, x.ProducerBrokerSettings);
  swap(id, x.id);
}

void Producer::poll() {
  int EventsHandled =
      rd_kafka_poll(RdKafkaPtr, ProducerBrokerSettings.PollTimeoutMS);
  LOG(Sev::Debug,
      "IID: {}  broker: {}  rd_kafka_poll()  served: {}  outq_len: {}", id,
      ProducerBrokerSettings.Address, EventsHandled, outputQueueLength());
  Stats.poll_served += EventsHandled;
  Stats.out_queue = outputQueueLength();
}

rd_kafka_t *Producer::getRdKafkaPtr() const { return RdKafkaPtr; }

uint64_t Producer::outputQueueLength() { return rd_kafka_outq_len(RdKafkaPtr); }

ProducerStats::ProducerStats(ProducerStats const &x) {
  produced = x.produced.load();
  produce_fail = x.produce_fail.load();
  local_queue_full = x.local_queue_full.load();
  produce_cb = x.produce_cb.load();
  produce_cb_fail = x.produce_cb_fail.load();
  poll_served = x.poll_served.load();
  msg_too_large = x.msg_too_large.load();
  produced_bytes = x.produced_bytes.load();
  out_queue = x.out_queue.load();
}
} // namespace KafkaW
