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

  virtual uint64_t outputQueueLength() = 0;
  virtual rd_kafka_s *getRdKafkaPtr() const = 0;
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
  std::function<void(rd_kafka_message_t const *msg)> on_delivery_ok;
  std::function<void(rd_kafka_message_t const *msg)> on_delivery_failed;
  // Currently it's nice to have access to these two for statistics:
  BrokerSettings ProducerBrokerSettings;
  RdKafka::Producer *RdKafkaPtr = nullptr;
  std::atomic<uint64_t> TotalMessagesProduced{0};
  ProducerStats Stats;

private:
  int id = 0;
};
} // namespace KafkaW
