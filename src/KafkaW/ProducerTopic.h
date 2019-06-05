#pragma once

#include "../logger.h"
#include "Producer.h"
#include "ProducerMessage.h"
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
  int produceAndSetKey(unsigned char *MsgData, size_t MsgSize,
                       const std::string &Key);
  int produce(std::unique_ptr<KafkaW::ProducerMessage> &Msg);
  std::string name() const;
  std::string brokerAddress() const;

private:
  std::shared_ptr<Producer> KafkaProducer;
  std::unique_ptr<RdKafka::Topic> RdKafkaTopic;
  std::string Name;
  std::unique_ptr<RdKafka::Conf> ConfigPtr{
      RdKafka::Conf::create(RdKafka::Conf::CONF_TOPIC)};
  bool DoCopyMsg{false};
  SharedLogger Logger = getLogger();
};
}
