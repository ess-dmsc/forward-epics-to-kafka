#include "Consumer.h"
#include "logger.h"
#include <atomic>
#include <helper.h>
#include <iostream>

namespace KafkaW {
//// C++READY
Consumer::Consumer(BrokerSettings BrokerSettings)
    : ConsumerBrokerSettings(std::move(BrokerSettings)) {
  std::string ErrStr;
  // create conf
  auto conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
  // set callbacks
  /// 'consume callback' was set to nullptr so I ommited it.
  conf->set("event_cb", &EventCallback, ErrStr);
  conf->set("rebalance_cb", &RebalanceCallback, ErrStr);

  // apply settings
  conf->set("group.id",
            fmt::format("forwarder-command-listener--pid{}", getpid()), ErrStr);
  ConsumerBrokerSettings.apply(conf);
  // create consumer
  this->KafkaConsumer = std::shared_ptr<RdKafka::KafkaConsumer>(
      RdKafka::KafkaConsumer::create(conf, ErrStr));
  if (!this->KafkaConsumer) {
    LOG(Sev::Error, "can not create kafka consumer: {}", ErrStr);
    throw std::runtime_error("can not create Kafka consumer");
  }
}

//// C++READY
Consumer::~Consumer() {
  LOG(Sev::Debug, "~Consumer()");
  if (KafkaConsumer) {
    LOG(Sev::Debug, "Close the consumer");
    this->KafkaConsumer->close();
    RdKafka::wait_destroyed(5000);
  }
}

//// C++READY
void Consumer::addTopic(std::string Topic) {
  LOG(Sev::Info, "Consumer::add_topic  {}", Topic);
  SubscribedTopics.push_back(Topic);

  // TODO: use ->assign() instead of ->subscribe like in kafkacow
  RdKafka::ErrorCode ERR = KafkaConsumer->subscribe(SubscribedTopics);
  if (ERR != 0) {
    LOG(Sev::Error, "could not subscribe to {}", Topic);
    throw std::runtime_error(fmt::format("could not subscribe to {}", Topic));
  }
}

//// C++READY
std::unique_ptr<Message> Consumer::poll() {
  auto KafkaMsg =
      std::unique_ptr<RdKafka::Message>(KafkaConsumer->consume(1000));
  switch (KafkaMsg->err()) {
  case RdKafka::ERR_NO_ERROR:
    // Real message
    if (KafkaMsg->len() > 0) {
      return make_unique<Message>((std::uint8_t *)KafkaMsg->payload(),
                                  KafkaMsg->len(), PollStatus::Msg);
    } else {
      return make_unique<Message>(PollStatus::Empty);
    }
  case RdKafka::ERR__PARTITION_EOF:
    return make_unique<Message>(PollStatus::EOP);
  default:
    /* All other errors */
    return make_unique<Message>(PollStatus::Err);
  }
}
} // namespace KafkaW
