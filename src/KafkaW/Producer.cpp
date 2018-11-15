#include "Producer.h"
#include "logger.h"

namespace KafkaW {

static std::atomic<int> g_kafka_producer_instance_count;

void ProducerMsg::deliveryOk() {}

void ProducerMsg::deliveryError() {}

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

void Producer::errorCallback(rd_kafka_t *RK, int Err_i, char const *Message,
                             void *Opaque) {
  UNUSED_ARG(RK);
  auto Self = reinterpret_cast<Producer *>(Opaque);
  auto ERR = static_cast<rd_kafka_resp_err_t>(Err_i);
  Sev Level = Sev::Warning;
  if (ERR == RD_KAFKA_RESP_ERR__TRANSPORT) {
    Level = Sev::Error;
  } else {
    if (Self->on_error)
      Self->on_error(Self, ERR);
  }
  LOG(Level, "Kafka cb_error id: {}  broker: {}  errno: {}  errorname: {}  "
             "errorstring: {}  message: {}",
      Self->id, Self->ProducerBrokerSettings.Address, Err_i,
      rd_kafka_err2name(ERR), rd_kafka_err2str(ERR), Message);
}

int Producer::statsCallback(rd_kafka_t *RK, char *Json, size_t Json_len,
                            void *Opaque) {
  auto Self = reinterpret_cast<Producer *>(Opaque);
  LOG(Sev::Debug, "IID: {}  INFO cb_stats {} length {}   {:.{}}", Self->id,
      rd_kafka_name(RK), Json_len, Json, Json_len);
  // What does librdkafka want us to return from this callback?
  return 0;
}

void Producer::logCallback(rd_kafka_t const *RK, int Level, char const *Fac,
                           char const *Buf) {
  UNUSED_ARG(Level);
  auto self = reinterpret_cast<Producer *>(rd_kafka_opaque(RK));
  LOG(Sev::Debug, "IID: {}  {}  fac: {}", self->id, Buf, Fac);
}

void Producer::throttleCallback(rd_kafka_t *RK, char const *Name,
                                int32_t Broker_id, int Throttle_time_ms,
                                void *Opaque) {
  UNUSED_ARG(RK);
  auto Self = reinterpret_cast<Producer *>(Opaque);
  LOG(Sev::Debug, "IID: {}  INFO cb_throttle  broker_id: {}  broker_name: {}  "
                  "throttle_time_ms: {}",
      Self->id, Broker_id, Name, Throttle_time_ms);
}

Producer::~Producer() {
  LOG(Sev::Debug, "~Producer");
  if (RdKafkaPtr) {
    int Timeout_ms = 1;
    uint32_t OutQueueLength = 0;
    while (true) {
      OutQueueLength = rd_kafka_outq_len(RdKafkaPtr);
      if (OutQueueLength == 0) {
        break;
      }
      auto EventsHandled = rd_kafka_poll(RdKafkaPtr, Timeout_ms);
      if (EventsHandled > 0) {
        LOG(Sev::Debug,
            "rd_kafka_poll handled: {}  outq before: {}  timeout: {}",
            EventsHandled, OutQueueLength, Timeout_ms);
      }
      Timeout_ms = Timeout_ms << 1;
      if (Timeout_ms > 8 * 1024) {
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

  rd_kafka_conf_t *Config = nullptr;
  Config = rd_kafka_conf_new();
  rd_kafka_conf_set_dr_msg_cb(Config, Producer::deliveredCallback);
  rd_kafka_conf_set_error_cb(Config, Producer::errorCallback);
  rd_kafka_conf_set_stats_cb(Config, Producer::statsCallback);
  rd_kafka_conf_set_log_cb(Config, Producer::logCallback);
  rd_kafka_conf_set_throttle_cb(Config, Producer::throttleCallback);

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
  if (log_level >= 8) {
    rd_kafka_dump(stdout, RdKafkaPtr);
  }
  Stats.poll_served += EventsHandled;
  Stats.out_queue = outputQueueLength();
}

void Producer::pollWhileOutputQueueFilled() {
  while (outputQueueLength() > 0) {
    Stats.poll_served +=
        rd_kafka_poll(RdKafkaPtr, ProducerBrokerSettings.PollTimeoutMS);
  }
}

rd_kafka_t *Producer::getRdKafkaPtr() const { return RdKafkaPtr; }

uint64_t Producer::outputQueueLength() { return rd_kafka_outq_len(RdKafkaPtr); }

uint64_t Producer::totalMessagesProduced() { return TotalMessagesProduced; }

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
