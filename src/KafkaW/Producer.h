#pragma once

#include "BrokerSettings.h"
#include "KafkaEventCb.h"
#include "ProducerDeliveryCb.h"
#include "ProducerMessage.h"
#include "ProducerStats.h"
#include <atomic>
#include <functional>

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
};

class Producer : public ProducerInterface {
public:
  explicit Producer(BrokerSettings ProducerBrokerSettings_);
  ~Producer() override;
  void poll() override;
  int outputQueueLength() override;
  RdKafka::Producer *getRdKafkaPtr() const override;
  RdKafka::ErrorCode produce(RdKafka::Topic *topic, int32_t partition,
                             int msgflags, void *payload, size_t len,
                             const void *key, size_t key_len, void *msg_opaque);
  // Currently it's nice to have access to these two for statistics:
  BrokerSettings ProducerBrokerSettings;
  std::atomic<uint64_t> TotalMessagesProduced{0};

protected:
  int id = 0;
  std::unique_ptr<RdKafka::Handle> ProducerPtr = nullptr;

private:
  std::unique_ptr<RdKafka::Conf> Conf;
  ProducerDeliveryCb DeliveryCb{Stats};
  KafkaEventCb EventCb;
};
} // namespace KafkaW
