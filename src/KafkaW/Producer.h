#pragma once

#include "BrokerSettings.h"
#include "ConsumerMessage.h"
#include "ProducerStats.h"
#include <atomic>
#include <functional>
#include <librdkafka/rdkafkacpp.h>

namespace KafkaW {

class ProducerTopic;

class ProducerInterface {
public:
  ProducerInterface() = default;
  virtual ~ProducerInterface() = default;
  virtual void poll() = 0;
  virtual int outputQueueLength() = 0;
  virtual RdKafka::Producer *getRdKafkaPtr() const = 0;
  ProducerStats Stats;
  std::function<void(RdKafka::Message const *msg)> on_delivery_ok = nullptr;
  std::function<void(RdKafka::Message const *msg)> on_delivery_failed = nullptr;
};

class Producer : public ProducerInterface {
public:
  typedef ProducerTopic Topic;
  explicit Producer(BrokerSettings ProducerBrokerSettings_);
  Producer(Producer const &) = delete;
  Producer(Producer &&x) noexcept;
  ~Producer() override;
  void poll() override;
  int outputQueueLength() override;
  RdKafka::Producer *getRdKafkaPtr() const override;
  // Currently it's nice to have access to these two for statistics:
  BrokerSettings ProducerBrokerSettings;
  std::atomic<uint64_t> TotalMessagesProduced{0};

private:
  RdKafka::Producer *ProducerPtr = nullptr;
  int id = 0;
};
} // namespace KafkaW
