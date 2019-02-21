#pragma once

#include "BrokerSettings.h"
#include "ConsumerMessage.h"
#include "KafkaEventCb.h"
#include "helper.h"
#include <vector>

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
  /// The constructor.
  ///
  /// \param Settings The broker settings.
  explicit Consumer(BrokerSettings &Settings);
  Consumer(Consumer &&) = delete;
  Consumer(Consumer const &) = delete;
  ~Consumer() override;

  /// Adds topic to consumer.
  ///
  /// \param Topic The topic name.
  void addTopic(const std::string &Topic) override;

  /// Polls for new messages.
  ///
  /// \return Any new messages received.
  std::unique_ptr<ConsumerMessage> poll() override;

protected:
  std::unique_ptr<RdKafka::KafkaConsumer> KafkaConsumer;

private:
  std::unique_ptr<RdKafka::Metadata> Metadata;
  std::unique_ptr<RdKafka::Conf> Conf;
  BrokerSettings ConsumerBrokerSettings;
  KafkaEventCb EventCallback;

  /// Get all partition numbers for a topic.
  ///
  /// \param Topic The topic name.
  /// \return A sorted list of all partitions on a topic.
  virtual std::vector<int32_t>
  getTopicPartitionNumbers(const std::string &Topic);

  /// Get the specified topic's metadata.
  ///
  /// \param Topic The topic name.
  /// \return The metadata.
  const RdKafka::TopicMetadata *findTopic(const std::string &Topic);

  /// Update the stored metadata.
  void updateMetadata();
};
} // namespace KafkaW
