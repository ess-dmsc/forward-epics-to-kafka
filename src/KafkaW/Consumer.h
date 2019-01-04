#pragma once

#include "BrokerSettings.h"
#include "KafkaEventCb.h"
#include "ConsumerMessage.h"
#include "helper.h"
#include <vector>

namespace KafkaW {

class ConsumerInterface {
public:
  ConsumerInterface() = default;
  virtual ~ConsumerInterface() = default;
  virtual void addTopic(const std::string &Topic) = 0;
  virtual std::unique_ptr<ConsumerMessage> poll() = 0;
};

class Consumer : public ConsumerInterface {
public:
  explicit Consumer(BrokerSettings &opt);
  Consumer(Consumer &&) = delete;
  Consumer(Consumer const &) = delete;
  ~Consumer() override;
  void addTopic(const std::string &Topic) override;
  std::unique_ptr<ConsumerMessage> poll() override;

private:
  std::vector<int32_t> getTopicPartitionNumbers(const std::string &Topic);
  std::unique_ptr<RdKafka::Metadata> queryMetadata();
  std::shared_ptr<RdKafka::KafkaConsumer> KafkaConsumer;
  BrokerSettings ConsumerBrokerSettings;
  KafkaEventCb EventCallback;
};
} // namespace KafkaW
