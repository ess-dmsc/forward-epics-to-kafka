// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

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

  virtual RdKafka::ErrorCode produce(RdKafka::Topic *Topic, int32_t Partition,
                                     int MessageFlags, void *Payload,
                                     size_t PayloadSize, const void *Key,
                                     size_t KeySize, void *OpaqueMessage) = 0;
  ProducerStats Stats;
};

class Producer : public ProducerInterface {
public:
  /// The constructor.
  ///
  /// \param Settings_ The BrokerSettings.
  explicit Producer(BrokerSettings Settings);
  ~Producer() override;

  /// Polls Kafka for events.
  void poll() override;

  /// Gets the number of messages not send.
  ///
  /// \return The number of messages.
  int outputQueueLength() override;

  /// Send a message to Kafka.
  ///
  /// \param Topic The topic to publish to.
  /// \param Partition The topic partition to publish to.
  /// \param MessageFlags
  /// \param Payload The actual message data.
  /// \param PayloadSize The size of the payload.
  /// \param Key The message's key.
  /// \param KeySize The size of the key.
  /// \param OpaqueMessage Points to the whole message.
  /// \return The Kafka RESP error code.
  RdKafka::ErrorCode produce(RdKafka::Topic *Topic, int32_t Partition,
                             int MessageFlags, void *Payload,
                             size_t PayloadSize, const void *Key,
                             size_t KeySize, void *OpaqueMessage) override;

  std::unique_ptr<RdKafka::Topic> createTopic(const std::string &TopicString,
                                              std::string &ErrStr);

  BrokerSettings ProducerBrokerSettings;
  std::atomic<uint64_t> TotalMessagesProduced{0};

protected:
  int ProducerID = 0;
  std::unique_ptr<RdKafka::Handle> ProducerPtr = nullptr;

private:
  std::unique_ptr<RdKafka::Conf> GlobalConf{
      RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL)};
  std::unique_ptr<RdKafka::Conf> TopicConf{
      RdKafka::Conf::create(RdKafka::Conf::CONF_TOPIC)};
  ProducerDeliveryCb DeliveryCb{Stats};
  KafkaEventCb EventCb;
  SharedLogger Logger = getLogger();
};
} // namespace KafkaW
