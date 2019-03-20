#pragma once

#include "Producer.h"
#include "ProducerMessage.h"
#include "logger.h"
#include <memory>
#include <string>

namespace KafkaW {

class TopicCreationError : public std::runtime_error {
public:
  TopicCreationError() : std::runtime_error("Can not create Kafka topic") {}
};

class ProducerTopic {
public:
  ProducerTopic(ProducerTopic &&) noexcept;
  ProducerTopic(std::shared_ptr<Producer> ProducerPtr, std::string TopicName);
  ~ProducerTopic() = default;
  int produce(unsigned char *MsgData, size_t MsgSize);
  int produce(std::unique_ptr<KafkaW::ProducerMessage> &Msg);
  void enableCopy();
  std::string name() const;
  std::string brokerAddress() const;

private:
  std::shared_ptr<Producer> KafkaProducer;
  std::unique_ptr<RdKafka::Topic> RdKafkaTopic;
  std::string Name;
  std::unique_ptr<RdKafka::Conf> ConfigPtr;

  bool DoCopyMsg{false};
};
}
