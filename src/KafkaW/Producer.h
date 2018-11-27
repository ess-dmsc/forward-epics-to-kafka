#pragma once

#include "BrokerSettings.h"
#include "ConsumerMessage.h"
#include "ProducerStats.h"
#include <atomic>
#include <functional>
#include <librdkafka/rdkafkacpp.h>
#include <librdkafka/rdkafka.h>

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
  uint64_t outputQueueLength() override;
  rd_kafka_t *getRdKafkaPtr() const override;
  std::function<void(rd_kafka_message_t const *msg)> on_delivery_ok;
  std::function<void(rd_kafka_message_t const *msg)> on_delivery_failed;
  std::function<void(ProducerInterface *, rd_kafka_resp_err_t)> on_error;
  // Currently it's nice to have access to these two for statistics:
  BrokerSettings ProducerBrokerSettings;
  rd_kafka_t *RdKafkaPtr = nullptr;
  std::atomic<uint64_t> TotalMessagesProduced{0};
  ProducerStats Stats;

private:
  int id = 0;
};
} // namespace KafkaW
