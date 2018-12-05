#include "Consumer.h"
#include "logger.h"
#include <algorithm>
#include <iostream>

namespace KafkaW {
//// C++READY
Consumer::Consumer(BrokerSettings BrokerSettings)
    : ConsumerBrokerSettings(std::move(BrokerSettings)) {
  std::string ErrStr;
  // create conf
  auto conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
  // set callbacks
  // 'consume callback' was set to nullptr so I omitted it.
  conf->set("event_cb", &EventCallback, ErrStr);
  conf->set("rebalance_cb", &RebalanceCallback, ErrStr);

  // apply settings
  conf->set("group.id",
            fmt::format("forwarder-command-listener--pid{}", getpid()), ErrStr);
  ConsumerBrokerSettings.apply(conf);
  // create consumer
  this->KafkaConsumer = std::shared_ptr<RdKafka::KafkaConsumer>(
      RdKafka::KafkaConsumer::create(conf, ErrStr));
  if (!this->KafkaConsumer) {
    LOG(Sev::Error, "can not create kafka consumer: {}", ErrStr);
    throw std::runtime_error("can not create Kafka consumer");
  }
  //  this->MetadataPointer = this->queryMetadata();
}

std::unique_ptr<RdKafka::Metadata> Consumer::queryMetadata() {
  RdKafka::Metadata *metadataRawPtr(nullptr);
  KafkaConsumer->metadata(true, nullptr, &metadataRawPtr, 1000);
  std::unique_ptr<RdKafka::Metadata> metadata(metadataRawPtr);
  try {
    if (!metadata) {
      throw std::runtime_error("Failed to query metadata from broker");
    }
  } catch (std::exception &E) {
    LOG(Sev::Error, "Failed to query metadata from broker: {}", E.what());
  }
  return metadata;
}

//// C++READY
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
  // save needed partition metadata here
  for (auto &Partition : *PartitionMetadata) {
    TopicPartitionNumbers.push_back(Partition->id());
  }
  sort(TopicPartitionNumbers.begin(), TopicPartitionNumbers.end());
  return TopicPartitionNumbers;
}

void Consumer::addTopic(std::string Topic) {
  LOG(Sev::Info, "Consumer::add_topic  {}", Topic);
  ////assign
  std::vector<RdKafka::TopicPartition *> TopicPartitionsWithOffsets;
  auto PartitionIDs = getTopicPartitionNumbers(Topic);
  for (unsigned long i = 0; i < PartitionIDs.size(); i++) {
    auto TopicPartition = RdKafka::TopicPartition::create(Topic, i);
    int64_t Low, High;
    KafkaConsumer->query_watermark_offsets(Topic, PartitionIDs[i], &Low, &High,
                                           100);
    TopicPartition->set_offset(Low);
    TopicPartitionsWithOffsets.push_back(TopicPartition);
  }
  RdKafka::ErrorCode ERR = KafkaConsumer->assign(TopicPartitionsWithOffsets);
  std::for_each(TopicPartitionsWithOffsets.cbegin(),
                TopicPartitionsWithOffsets.cend(),
                [](RdKafka::TopicPartition *Partition) { delete Partition; });
  ////ENDassign
  if (ERR != 0) {
    LOG(Sev::Error, "could not subscribe to {}", Topic);
    throw std::runtime_error(fmt::format("could not subscribe to {}", Topic));
  }
  SubscribedTopics.push_back(Topic);
}

//// C++READY
std::unique_ptr<Message> Consumer::poll() {
  auto KafkaMsg =
      std::unique_ptr<RdKafka::Message>(KafkaConsumer->consume(1000));
  switch (KafkaMsg->err()) {
  case RdKafka::ERR_NO_ERROR:
    if (KafkaMsg->len() > 0) {
      return make_unique<Message>((std::uint8_t *)KafkaMsg->payload(),
                                  KafkaMsg->len(), PollStatus::Msg);
    } else {
      return make_unique<Message>(PollStatus::Empty);
    }
  case RdKafka::ERR__PARTITION_EOF:
    return make_unique<Message>(PollStatus::EOP);
  default:
    return make_unique<Message>(PollStatus::Err);
  }
}
} // namespace KafkaW
