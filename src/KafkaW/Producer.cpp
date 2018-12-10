#include "Producer.h"
#include "ProducerDeliveryCb.h"
#include "ProducerEventCb.h"
#include "logger.h"

namespace KafkaW {

static std::atomic<int> g_kafka_producer_instance_count;

Producer::~Producer() {
  LOG(Sev::Debug, "~Producer");
  if (ProducerPtr) {
    int TimeoutMS = 1;
    int OutQueueLength = 0;
    while (true) {
      OutQueueLength = ProducerPtr->outq_len();
      if (OutQueueLength == 0) {
        break;
      }
      auto EventsHandled = ProducerPtr->poll(TimeoutMS);
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
    ProducerPtr = nullptr;
  }
}

Producer::Producer(BrokerSettings ProducerBrokerSettings)
    : ProducerBrokerSettings(ProducerBrokerSettings) {
  id = g_kafka_producer_instance_count++;

  // librdkafka API sometimes wants to write errors into a buffer:
  std::string errstr;

  auto Config = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
  ProducerBrokerSettings.apply(Config);
  ProducerDeliveryCb DeliveryCallback(std::make_shared<Producer>(*this));
  ProducerEventCb EventCallback;
  Config->set("dr_cb", &DeliveryCallback, errstr);
  Config->set("event_cb", &EventCallback, errstr);
  //  Config->set("metadata.broker.list", ProducerBrokerSettings.Address,
  //   errstr); //TODO: for some reason this seems to cause a segfault?

  LOG(Sev::Debug, "Producer opaque: {}", (void *)this);

  ProducerPtr = RdKafka::Producer::create(Config, errstr);
  if (!ProducerPtr) {
    LOG(Sev::Error, "can not create kafka handle: {}", errstr);
    throw std::runtime_error("can not create Kafka handle");
  }

  LOG(Sev::Info, "New Kafka {} with brokers: {}", ProducerPtr->name(),
      ProducerBrokerSettings.Address.c_str());
}

Producer::Producer(Producer &x) noexcept {
  using std::swap;
  swap(ProducerPtr, x.ProducerPtr);
  swap(on_delivery_ok, x.on_delivery_ok);
  swap(on_delivery_failed, x.on_delivery_failed);
  swap(ProducerBrokerSettings, x.ProducerBrokerSettings);
  swap(id, x.id);
}

void Producer::poll() {
  auto EventsHandled = ProducerPtr->poll(ProducerBrokerSettings.PollTimeoutMS);
  LOG(Sev::Debug,
      "IID: {}  broker: {}  rd_kafka_poll()  served: {}  outq_len: {}", id,
      ProducerBrokerSettings.Address, EventsHandled, outputQueueLength());
  Stats.poll_served += EventsHandled;
  Stats.out_queue = outputQueueLength();
}

RdKafka::Producer *Producer::getRdKafkaPtr() const { return ProducerPtr; }

int Producer::outputQueueLength() { return ProducerPtr->outq_len(); }
} // namespace KafkaW
