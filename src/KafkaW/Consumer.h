#pragma once

#include "BrokerSettings.h"
#include "ConsumerEventCb.h"
#include "ConsumerRebalanceCb.h"
#include "Message.h"
#include <functional>
#include <librdkafka/rdkafka.h>
#include <memory>
#include <vector>

namespace KafkaW {

class ConsumerInterface {
public:
  ConsumerInterface() = default;
  virtual ~ConsumerInterface() = default;
  virtual void addTopic(std::string Topic) = 0;
  virtual std::unique_ptr<Message> poll() = 0;
};

class Consumer : public ConsumerInterface {
public:
  explicit Consumer(BrokerSettings opt);
  Consumer(Consumer &&) = delete;
  Consumer(Consumer const &) = delete;
  ~Consumer() override;
  void addTopic(std::string Topic) override;
  std::unique_ptr<Message> poll() override;

private:
  std::shared_ptr<RdKafka::KafkaConsumer> KafkaConsumer;
  BrokerSettings ConsumerBrokerSettings;
  std::vector<std::string> SubscribedTopics;
  ConsumerEventCb EventCallback;
  ConsumerRebalanceCb RebalanceCallback;
};
} // namespace KafkaW
