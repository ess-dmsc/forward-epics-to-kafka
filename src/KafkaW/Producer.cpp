#include "Producer.h"
#include "logger.h"
#include <vector>

namespace KafkaW {

static std::atomic<int> ProducerInstanceCount;

Producer::~Producer() {
  Logger->trace("~Producer");
  if (ProducerPtr != nullptr) {
    int TimeoutMS = 100;
    int NumberOfIterations = 80;
    for (int i = 0; i < NumberOfIterations; i++) {
      if (outputQueueLength() == 0) {
        break;
      }
      ProducerPtr->poll(TimeoutMS);
    }
    if (outputQueueLength() > 0) {
      Logger->info(
          "Kafka out queue still not empty: {}, destroying producer anyway.",
          outputQueueLength());
    }
  }
}

Producer::Producer(BrokerSettings Settings)
    : ProducerBrokerSettings(std::move(Settings)),
      Conf(std::unique_ptr<RdKafka::Conf>(
          RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL))) {
  ProducerID = ProducerInstanceCount++;

  std::string ErrorString;

  try {
    ProducerBrokerSettings.apply(Conf.get());
  } catch (std::runtime_error &e) {
    throw std::runtime_error(
        "Cannot create kafka handle due to configuration error");
  }

  Conf->set("dr_cb", &DeliveryCb, ErrorString);
  Conf->set("event_cb", &EventCb, ErrorString);
  Conf->set("metadata.broker.list", ProducerBrokerSettings.Address,
            ErrorString);
  ProducerPtr.reset(RdKafka::Producer::create(Conf.get(), ErrorString));
  if (!ProducerPtr) {
    Logger->error("can not create kafka handle: {}", ErrorString);
    throw std::runtime_error("can not create Kafka handle");
  }

  Logger->info("new Kafka producer: {}, with brokers: {}", ProducerPtr->name(),
               ProducerBrokerSettings.Address.c_str());
}

void Producer::poll() {
  auto EventsHandled = ProducerPtr->poll(ProducerBrokerSettings.PollTimeoutMS);
  Logger->debug(
      "IID: {}  broker: {}  rd_kafka_poll()  served: {}  outq_len: {}",
      ProducerID, ProducerBrokerSettings.Address, EventsHandled,
      outputQueueLength());
  Stats.poll_served += EventsHandled;
  Stats.out_queue = outputQueueLength();
}

RdKafka::Producer *Producer::getRdKafkaPtr() const {
  return dynamic_cast<RdKafka::Producer *>(ProducerPtr.get());
}

int Producer::outputQueueLength() { return ProducerPtr->outq_len(); }

RdKafka::ErrorCode Producer::produce(RdKafka::Topic *Topic, int32_t Partition,
                                     int MessageFlags, void *Payload,
                                     size_t PayloadSize, const void *Key,
                                     size_t KeySize, void *OpaqueMessage) {
  // Do a non-blocking poll of the local producer (note this is not polling
  // anything across the network)
  // NB, if we don't call poll then we haven't handled successful publishing of
  // each message and the messages therefore never get removed from librdkafka's
  // producer queue
  ProducerPtr->poll(0);

  return dynamic_cast<RdKafka::Producer *>(ProducerPtr.get())
      ->produce(Topic, Partition, MessageFlags, Payload, PayloadSize, Key,
                KeySize, OpaqueMessage);
}
} // namespace KafkaW
