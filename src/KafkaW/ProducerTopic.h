#pragma once

#include "Message.h"
#include "Producer.h"
#include "TopicSettings.h"
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
  ProducerTopic(ProducerTopic &&);
  ProducerTopic(std::shared_ptr<Producer> Producer_, std::string Name_);
  ~ProducerTopic();
  int produce(unsigned char *MsgData, size_t MsgSize);
  int produce(std::unique_ptr<Producer::Msg> &Msg);
  // Currently it's nice to have access to these for statistics:
  std::shared_ptr<Producer> Producer_;
  rd_kafka_topic_t *RdKafkaTopic = nullptr;
  void enableCopy();
  std::string name() const;

private:
  std::string Name;
  bool DoCopyMsg{false};
};
}
