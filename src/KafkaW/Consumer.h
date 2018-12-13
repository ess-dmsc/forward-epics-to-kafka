#pragma once

#include "BrokerSettings.h"
#include "ConsumerEventCb.h"
#include "ConsumerMessage.h"
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
  std::shared_ptr<RdKafka::KafkaConsumer> KafkaConsumer;
  BrokerSettings ConsumerBrokerSettings;
  ConsumerEventCb EventCallback;
};
} // namespace KafkaW
