#include "Consumer.h"
#include "MetadataException.h"
#include "logger.h"
#include <algorithm>
#include <iostream>

namespace KafkaW {
Consumer::Consumer(BrokerSettings &BrokerSettings)
    : ConsumerBrokerSettings(std::move(BrokerSettings)) {
  std::string ErrorString;
  auto conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
  conf->set("event_cb", &EventCallback, ErrorString);
  conf->set("rebalance_cb", &RebalanceCallback, ErrorString);

  conf->set("group.id",
            fmt::format("forwarder-command-listener--pid{}", getpid()),
            ErrorString);
  ConsumerBrokerSettings.apply(conf);
  this->KafkaConsumer = std::shared_ptr<RdKafka::KafkaConsumer>(
      RdKafka::KafkaConsumer::create(conf, ErrorString));
  if (!this->KafkaConsumer) {
    LOG(Sev::Error, "can not create kafka consumer: {}", ErrorString);
    throw std::runtime_error("can not create Kafka consumer");
  }
}

std::unique_ptr<RdKafka::Metadata> Consumer::queryMetadata() {
  RdKafka::Metadata *metadataRawPtr(nullptr);
  KafkaConsumer->metadata(true, nullptr, &metadataRawPtr, 1000);
  std::unique_ptr<RdKafka::Metadata> metadata(metadataRawPtr);
  if (!metadata) {
    throw MetadataException(
        "Consumer::queryMetadata() - error while retrieving metadata.");
  }
  delete metadataRawPtr;
  return metadata;
}

Consumer::~Consumer() {
  LOG(Sev::Debug, "~Consumer()");
  if (KafkaConsumer) {
    LOG(Sev::Debug, "Close the consumer");
    this->KafkaConsumer->close();
    RdKafka::wait_destroyed(5000);
  }
}

std::vector<int32_t>
Consumer::getTopicPartitionNumbers(const std::string &Topic) {
  this->MetadataPointer = queryMetadata();
  auto Topics = MetadataPointer->topics();
  auto Iterator = std::find_if(Topics->cbegin(), Topics->cend(),
                               [Topic](const RdKafka::TopicMetadata *tpc) {
                                 return tpc->topic() == Topic;
                               });
  auto matchedTopic = *Iterator;
  std::vector<int32_t> TopicPartitionNumbers;
  auto PartitionMetadata = matchedTopic->partitions();
  for (auto &Partition : *PartitionMetadata) {
    TopicPartitionNumbers.push_back(Partition->id());
  }
  sort(TopicPartitionNumbers.begin(), TopicPartitionNumbers.end());
  return TopicPartitionNumbers;
}

void Consumer::addTopic(std::string Topic) {
  LOG(Sev::Info, "Consumer::add_topic  {}", Topic);
  std::vector<RdKafka::TopicPartition *> TopicPartitionsWithOffsets;
  auto PartitionIDs = getTopicPartitionNumbers(Topic);
  for (int PartitionID : PartitionIDs) {
    auto TopicPartition = RdKafka::TopicPartition::create(Topic, PartitionID);
    int64_t Low, High;
    KafkaConsumer->query_watermark_offsets(Topic, PartitionID, &Low, &High,
                                           100);
    TopicPartition->set_offset(Low);
    TopicPartitionsWithOffsets.push_back(TopicPartition);
  }
  RdKafka::ErrorCode ERR = KafkaConsumer->assign(TopicPartitionsWithOffsets);
  std::for_each(TopicPartitionsWithOffsets.cbegin(),
                TopicPartitionsWithOffsets.cend(),
                [](RdKafka::TopicPartition *Partition) { delete Partition; });
  if (ERR != 0) {
    LOG(Sev::Error, "Could not subscribe to {}", Topic);
    throw std::runtime_error(fmt::format("Could not subscribe to {}", Topic));
  }
  SubscribedTopics.push_back(Topic);
}

std::unique_ptr<ConsumerMessage> Consumer::poll() {
  auto KafkaMsg =
      std::unique_ptr<RdKafka::Message>(KafkaConsumer->consume(1000));
  switch (KafkaMsg->err()) {
  case RdKafka::ERR_NO_ERROR:
    if (KafkaMsg->len() > 0) {
      return ::make_unique<ConsumerMessage>((std::uint8_t *)KafkaMsg->payload(),
                                            KafkaMsg->len(), PollStatus::Msg);
    } else {
      return ::make_unique<ConsumerMessage>(PollStatus::Empty);
    }
  case RdKafka::ERR__PARTITION_EOF:
    return ::make_unique<ConsumerMessage>(PollStatus::EOP);
  default:
    return ::make_unique<ConsumerMessage>(PollStatus::Err);
  }
}
} // namespace KafkaW
