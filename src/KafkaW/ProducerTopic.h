// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

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
  bool DoCopyMsg{false};
  SharedLogger Logger = getLogger();
};
} // namespace KafkaW
