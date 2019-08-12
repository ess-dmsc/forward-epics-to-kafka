// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

#include "Consumer.h"
#include "../logger.h"
#include "MetadataException.h"
#include <algorithm>
#include <iostream>
#ifdef _MSC_VER
#include "process.h"
#define getpid _getpid
#else
#include <unistd.h>
#endif

namespace KafkaW {
Consumer::Consumer(BrokerSettings &Settings)
    : ConsumerBrokerSettings(std::move(Settings)),
      Conf(std::unique_ptr<RdKafka::Conf>(
          RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL))) {
  std::string ErrorString;

  Conf->set("event_cb", &EventCallback, ErrorString);
  Conf->set("metadata.broker.list", ConsumerBrokerSettings.Address,
            ErrorString);
  Conf->set("group.id",
            fmt::format("forwarder-command-listener--pid{}", getpid()),
            ErrorString);
  ConsumerBrokerSettings.apply(Conf.get());

  KafkaConsumer = std::unique_ptr<RdKafka::KafkaConsumer>(
      RdKafka::KafkaConsumer::create(Conf.get(), ErrorString));
  if (!KafkaConsumer) {
    Logger->error("can not create kafka consumer: {}", ErrorString);
    throw std::runtime_error("can not create Kafka consumer");
  }
}

void Consumer::updateMetadata() {
  RdKafka::Metadata *MetadataPtr = nullptr;
  auto RetCode = KafkaConsumer->metadata(true, nullptr, &MetadataPtr, 5000);
  if (RetCode != RdKafka::ERR_NO_ERROR) {
    throw MetadataException(
        "Consumer::updateMetadata() - error while retrieving metadata.");
  }
  Metadata = std::unique_ptr<RdKafka::Metadata>(MetadataPtr);
}

Consumer::~Consumer() {
  Logger->trace("~Consumer()");
  if (KafkaConsumer != nullptr) {
    Logger->trace("Close the consumer");
    KafkaConsumer->close();
  }
}

const RdKafka::TopicMetadata *Consumer::findTopic(const std::string &Topic) {
  updateMetadata();
  auto Topics = Metadata->topics();
  auto Iterator =
      std::find_if(Topics->cbegin(), Topics->cend(),
                   [Topic](const RdKafka::TopicMetadata *TopicMetadata) {
                     return TopicMetadata->topic() == Topic;
                   });
  if (Iterator == Topics->end()) {
    throw std::runtime_error("Config topic does not exist");
  }
  return *Iterator;
}

std::vector<int32_t>
Consumer::getTopicPartitionNumbers(const std::string &Topic) {
  auto matchedTopic = findTopic(Topic);
  std::vector<int32_t> TopicPartitionNumbers;
  const RdKafka::TopicMetadata::PartitionMetadataVector *PartitionMetadata =
      matchedTopic->partitions();

  for (const auto &Partition : *PartitionMetadata) {
    // cppcheck-suppress useStlAlgorithm ; readability
    TopicPartitionNumbers.push_back(Partition->id());
  }

  sort(TopicPartitionNumbers.begin(), TopicPartitionNumbers.end());
  return TopicPartitionNumbers;
}

void Consumer::addTopic(const std::string &Topic) {
  Logger->info("Consumer::add_topic  {}", Topic);
  std::vector<RdKafka::TopicPartition *> TopicPartitionsWithOffsets;
  auto PartitionIDs = getTopicPartitionNumbers(Topic);
  for (int PartitionID : PartitionIDs) {
    auto TopicPartition = RdKafka::TopicPartition::create(Topic, PartitionID);
    int64_t Low, High;
    KafkaConsumer->query_watermark_offsets(Topic, PartitionID, &Low, &High,
                                           1000);
    TopicPartition->set_offset(High);
    TopicPartitionsWithOffsets.push_back(TopicPartition);
  }
  RdKafka::ErrorCode Err = KafkaConsumer->assign(TopicPartitionsWithOffsets);
  std::for_each(TopicPartitionsWithOffsets.cbegin(),
                TopicPartitionsWithOffsets.cend(),
                [](RdKafka::TopicPartition *Partition) { delete Partition; });
  if (Err != RdKafka::ERR_NO_ERROR) {
    Logger->error("Could not subscribe to {}", Topic);
    throw std::runtime_error(fmt::format("Could not subscribe to {}", Topic));
  }
}

std::unique_ptr<ConsumerMessage> Consumer::poll() {
  auto KafkaMsg = std::unique_ptr<RdKafka::Message>(
      KafkaConsumer->consume(ConsumerBrokerSettings.PollTimeoutMS));
  switch (KafkaMsg->err()) {
  case RdKafka::ERR_NO_ERROR:
    if (KafkaMsg->len() > 0) {
      std::string MessageString = {
          reinterpret_cast<const char *>(KafkaMsg->payload())};
      auto Message =
          ::make_unique<ConsumerMessage>(MessageString, PollStatus::Message);
      return Message;
    } else {
      return ::make_unique<ConsumerMessage>(PollStatus::Empty);
    }
  case RdKafka::ERR__PARTITION_EOF:
    return ::make_unique<ConsumerMessage>(PollStatus::EndOfPartition);
  default:
    return ::make_unique<ConsumerMessage>(PollStatus::Error);
  }
}
} // namespace KafkaW
