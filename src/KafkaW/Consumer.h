#pragma once

#include "BrokerSettings.h"
#include "ConsumerEventCb.h"
#include "ConsumerMessage.h"
#include "ConsumerRebalanceCb.h"
#include "helper.h"
#include <vector>
#ifdef _MSC_VER
#include "process.h"
#define getpid _getpid
#else
#include "ConsumerEventCb.h"
#include <unistd.h>
#endif

namespace KafkaW {

class ConsumerInterface {
public:
  ConsumerInterface() = default;
  virtual ~ConsumerInterface() = default;
  virtual void addTopic(std::string Topic) = 0;
  virtual std::unique_ptr<ConsumerMessage> poll() = 0;
};

class Consumer : public ConsumerInterface {
public:
  explicit Consumer(BrokerSettings &opt);
  Consumer(Consumer &&) = delete;
  Consumer(Consumer const &) = delete;
  ~Consumer() override;
  void addTopic(std::string Topic) override;
  std::unique_ptr<ConsumerMessage> poll() override;

private:
  std::vector<int32_t> getTopicPartitionNumbers(const std::string &Topic);
  std::unique_ptr<RdKafka::Metadata> queryMetadata();
  std::shared_ptr<RdKafka::KafkaConsumer> KafkaConsumer;
  BrokerSettings ConsumerBrokerSettings;
  std::vector<std::string> SubscribedTopics;
  ConsumerEventCb EventCallback;
  ConsumerRebalanceCb RebalanceCallback;
  std::unique_ptr<RdKafka::Metadata> MetadataPointer = nullptr;
};
} // namespace KafkaW
