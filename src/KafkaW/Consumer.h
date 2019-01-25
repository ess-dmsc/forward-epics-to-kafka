#pragma once

#include "BrokerSettings.h"
#include "ConsumerMessage.h"
#include "KafkaEventCb.h"
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

protected:
  std::unique_ptr<RdKafka::KafkaConsumer> KafkaConsumer;

private:
  std::unique_ptr<RdKafka::Metadata> Metadata;
  std::unique_ptr<RdKafka::Conf> Conf;
  BrokerSettings ConsumerBrokerSettings;
  KafkaEventCb EventCallback;
  virtual std::vector<int32_t>
  getTopicPartitionNumbers(const std::string &Topic);
  const RdKafka::TopicMetadata *findTopic(const std::string &Topic);
  void updateMetadata();
};
} // namespace KafkaW
