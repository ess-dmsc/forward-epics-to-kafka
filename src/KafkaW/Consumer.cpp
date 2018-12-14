#include "Consumer.h"
#include "MetadataException.h"
#include "logger.h"
#include <algorithm>
#include <iostream>

namespace KafkaW {
Consumer::Consumer(BrokerSettings &BrokerSettings)
    : ConsumerBrokerSettings(std::move(BrokerSettings)) {
  std::string ErrorString;
  auto conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
  conf->set("event_cb", &EventCallback, ErrorString);
  conf->set("metadata.broker.list", ConsumerBrokerSettings.Address,
            ErrorString);
  conf->set("group.id",
            fmt::format("forwarder-command-listener--pid{}", getpid()),
            ErrorString);
  ConsumerBrokerSettings.apply(conf);
  this->KafkaConsumer = std::shared_ptr<RdKafka::KafkaConsumer>(
      RdKafka::KafkaConsumer::create(conf, ErrorString));
  if (!this->KafkaConsumer) {
    LOG(Sev::Error, "can not create kafka consumer: {}", ErrorString);
    throw std::runtime_error("can not create Kafka consumer");
  }
}

Consumer::~Consumer() {
  LOG(Sev::Debug, "~Consumer()");
  if (KafkaConsumer) {
    LOG(Sev::Debug, "Close the consumer");
    this->KafkaConsumer->close();
    RdKafka::wait_destroyed(5000);
  }
}

void Consumer::addTopic(const std::string &Topic) {
  auto ErrCode = KafkaConsumer->subscribe({Topic});
  if (ErrCode != RdKafka::ErrorCode::ERR_NO_ERROR) {
    LOG(Sev::Warning, "Unable to subscribe to topic {} - {}", Topic,
        RdKafka::err2str(ErrCode));
  };
}

std::unique_ptr<ConsumerMessage> Consumer::poll() {
  auto KafkaMsg = std::unique_ptr<RdKafka::Message>(
      KafkaConsumer->consume(ConsumerBrokerSettings.PollTimeoutMS));
  switch (KafkaMsg->err()) {
  case RdKafka::ERR_NO_ERROR:
    if (KafkaMsg->len() > 0) {
      return ::make_unique<ConsumerMessage>((std::uint8_t *)KafkaMsg->payload(),
                                            KafkaMsg->len(), PollStatus::Msg);
    } else {
      return ::make_unique<ConsumerMessage>(PollStatus::Empty);
    }
  case RdKafka::ERR__PARTITION_EOF:
    return ::make_unique<ConsumerMessage>(PollStatus::EOP);
  default:
    return ::make_unique<ConsumerMessage>(PollStatus::Err);
  }
}
} // namespace KafkaW
