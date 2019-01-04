#include "Consumer.h"
#include "MetadataException.h"
#include "logger.h"
#include <algorithm>
#include <iostream>
#ifdef _MSC_VER
#include "process.h"
#define getpid _getpid
#else
#include <unistd.h>
#endif

namespace KafkaW {
Consumer::Consumer(BrokerSettings &BrokerSettings)
    : ConsumerBrokerSettings(std::move(BrokerSettings)) {
  std::string ErrorString;

  auto conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
  conf->set("event_cb", &EventCallback, ErrorString);
  conf->set("metadata.broker.list", ConsumerBrokerSettings.Address,
            ErrorString);
  conf->set("group.id",
            fmt::format("forwarder-command-listener--pid{}", getpid()),
            ErrorString);
  ConsumerBrokerSettings.apply(conf);

  KafkaConsumer = std::shared_ptr<RdKafka::KafkaConsumer>(
      RdKafka::KafkaConsumer::create(conf, ErrorString));
  if (!KafkaConsumer) {
    LOG(Sev::Error, "can not create kafka consumer: {}", ErrorString);
    throw std::runtime_error("can not create Kafka consumer");
  }
}

std::unique_ptr<RdKafka::Metadata> Consumer::queryMetadata() {
  RdKafka::Metadata *metadataRawPtr;
  auto RetCode = KafkaConsumer->metadata(true, nullptr, &metadataRawPtr, 5000);
  std::unique_ptr<RdKafka::Metadata> metadata(metadataRawPtr);
  if (RetCode != RdKafka::ERR_NO_ERROR) {
    throw MetadataException(
        "Consumer::queryMetadata() - error while retrieving metadata.");
  }
  return metadata;
}

Consumer::~Consumer() {
  LOG(Sev::Debug, "~Consumer()");
  if (KafkaConsumer != nullptr) {
    LOG(Sev::Debug, "Close the consumer");
    KafkaConsumer->close();
    RdKafka::wait_destroyed(5000);
  }
}

/// Get a vector of partition numbers which exist for a given topic
/// \param Topic Name of the topic
/// \return Vector of partition numbers
std::vector<int32_t>
Consumer::getTopicPartitionNumbers(const std::string &Topic) {
  auto MetadataPtr = queryMetadata();
  auto Topics = MetadataPtr->topics();
  auto Iterator = std::find_if(Topics->cbegin(), Topics->cend(),
                               [Topic](const RdKafka::TopicMetadata *tpc) {
                                 return tpc->topic() == Topic;
                               });
  if (Iterator == Topics->end()) {
    throw std::runtime_error("Config topic does not exist");
  }
  auto matchedTopic = *Iterator;
  std::vector<int32_t> TopicPartitionNumbers;
  auto PartitionMetadata = matchedTopic->partitions();
  for (const auto &Partition : *PartitionMetadata) {
    TopicPartitionNumbers.push_back(Partition->id());
  }
  sort(TopicPartitionNumbers.begin(), TopicPartitionNumbers.end());
  return TopicPartitionNumbers;
}

/// Subscribe to specified topic, note this removes any previous subscription
/// \param Topic Name of the topic to subscribe to
void Consumer::addTopic(const std::string &Topic) {
  LOG(Sev::Info, "Consumer::add_topic  {}", Topic);
  std::vector<RdKafka::TopicPartition *> TopicPartitionsWithOffsets;
  auto PartitionIDs = getTopicPartitionNumbers(Topic);
  for (int PartitionID : PartitionIDs) {
    auto TopicPartition = RdKafka::TopicPartition::create(Topic, PartitionID);
    int64_t Low, High;
    KafkaConsumer->query_watermark_offsets(Topic, PartitionID, &Low, &High,
                                           1000);
    TopicPartition->set_offset(Low);
    TopicPartitionsWithOffsets.push_back(TopicPartition);
  }
  RdKafka::ErrorCode Err = KafkaConsumer->assign(TopicPartitionsWithOffsets);
  std::for_each(TopicPartitionsWithOffsets.cbegin(),
                TopicPartitionsWithOffsets.cend(),
                [](RdKafka::TopicPartition *Partition) { delete Partition; });
  if (Err != RdKafka::ERR_NO_ERROR) {
    LOG(Sev::Error, "Could not subscribe to {}", Topic);
    throw std::runtime_error(fmt::format("Could not subscribe to {}", Topic));
  }
}

std::unique_ptr<ConsumerMessage> Consumer::poll() {
  auto KafkaMsg = std::unique_ptr<RdKafka::Message>(
      KafkaConsumer->consume(ConsumerBrokerSettings.PollTimeoutMS));
  switch (KafkaMsg->err()) {
  case RdKafka::ERR_NO_ERROR:
    if (KafkaMsg->len() > 0) {
      std::string MessageString = {reinterpret_cast<const char *>(KafkaMsg->payload())};
      auto Message = ::make_unique<ConsumerMessage>(MessageString, PollStatus::Msg);
      return Message;
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
