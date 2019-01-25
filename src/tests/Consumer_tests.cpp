#include "../KafkaW/Consumer.h"
#include "MockMessage.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <librdkafka/rdkafkacpp.h>
using ::testing::AtLeast;
using ::testing::Exactly;
using ::testing::Return;
using ::testing::_;
using ::testing::SetArgPointee;

using namespace KafkaW;

class MockKafkaConsumer : public RdKafka::KafkaConsumer {
public:
  MOCK_CONST_METHOD0(name, const std::string());
  MOCK_CONST_METHOD0(memberid, const std::string());
  MOCK_METHOD1(poll, int(int));
  MOCK_METHOD0(outq_len, int());
  MOCK_METHOD4(metadata, RdKafka::ErrorCode(bool, const RdKafka::Topic *,
                                            RdKafka::Metadata **, int));
  MOCK_METHOD1(pause,
               RdKafka::ErrorCode(std::vector<RdKafka::TopicPartition *> &));
  MOCK_METHOD1(resume,
               RdKafka::ErrorCode(std::vector<RdKafka::TopicPartition *> &));
  MOCK_METHOD5(query_watermark_offsets,
               RdKafka::ErrorCode(const std::string &, int32_t, int64_t *,
                                  int64_t *, int));
  MOCK_METHOD4(get_watermark_offsets,
               RdKafka::ErrorCode(const std::string &, int32_t, int64_t *,
                                  int64_t *));
  MOCK_METHOD2(offsetsForTimes,
               RdKafka::ErrorCode(std::vector<RdKafka::TopicPartition *> &,
                                  int));
  MOCK_METHOD1(get_partition_queue,
               RdKafka::Queue *(const RdKafka::TopicPartition *));
  MOCK_METHOD1(set_log_queue, RdKafka::ErrorCode(RdKafka::Queue *));
  MOCK_METHOD0(yield, void());
  MOCK_METHOD1(clusterid, const std::string(int));
  MOCK_METHOD0(c_ptr, rd_kafka_s *());
  MOCK_METHOD1(subscribe, RdKafka::ErrorCode(const std::vector<std::string> &));
  MOCK_METHOD0(unsubscribe, RdKafka::ErrorCode());
  MOCK_METHOD1(assign, RdKafka::ErrorCode(
                           const std::vector<RdKafka::TopicPartition *> &));
  MOCK_METHOD0(unassign, RdKafka::ErrorCode());
  MOCK_METHOD1(consume, RdKafka::Message *(int));
  MOCK_METHOD0(commitSync, RdKafka::ErrorCode());
  MOCK_METHOD0(commitAsync, RdKafka::ErrorCode());
  MOCK_METHOD1(commitSync, RdKafka::ErrorCode(RdKafka::Message *));
  MOCK_METHOD1(commitAsync, RdKafka::ErrorCode(RdKafka::Message *));
  MOCK_METHOD1(commitSync,
               RdKafka::ErrorCode(std::vector<RdKafka::TopicPartition *> &));
  MOCK_METHOD1(
      commitAsync,
      RdKafka::ErrorCode(const std::vector<RdKafka::TopicPartition *> &));
  MOCK_METHOD1(commitSync, RdKafka::ErrorCode(RdKafka::OffsetCommitCb *));
  MOCK_METHOD2(commitSync,
               RdKafka::ErrorCode(std::vector<RdKafka::TopicPartition *> &,
                                  RdKafka::OffsetCommitCb *));
  MOCK_METHOD2(committed,
               RdKafka::ErrorCode(std::vector<RdKafka::TopicPartition *> &,
                                  int));
  MOCK_METHOD1(position,
               RdKafka::ErrorCode(std::vector<RdKafka::TopicPartition *> &));
  MOCK_METHOD0(close, RdKafka::ErrorCode());
  MOCK_METHOD2(seek, RdKafka::ErrorCode(const RdKafka::TopicPartition &, int));
  MOCK_METHOD1(offsets_store,
               RdKafka::ErrorCode(std::vector<RdKafka::TopicPartition *> &));
  MOCK_METHOD1(assignment,
               RdKafka::ErrorCode(std::vector<RdKafka::TopicPartition *> &));
  MOCK_METHOD1(subscription, RdKafka::ErrorCode(std::vector<std::string> &));
};

class MockMetadata : public RdKafka::Metadata {
public:
  MOCK_CONST_METHOD0(brokers,
                     const RdKafka::Metadata::BrokerMetadataVector *());
  MOCK_CONST_METHOD0(topics, const RdKafka::Metadata::TopicMetadataVector *());
  MOCK_CONST_METHOD0(orig_broker_id, int32_t());
  MOCK_CONST_METHOD0(orig_broker_name, const std::string());
};

class MockTopicMetadata : public RdKafka::TopicMetadata {
private:
  std::string name;

public:
  MockTopicMetadata(std::string name) : name(name) {}

  const std::string topic() const override { return name; }

  MOCK_CONST_METHOD0(partitions, PartitionMetadataVector *());
  MOCK_CONST_METHOD0(err, RdKafka::ErrorCode());
};

class MockPartitionMetadata : public RdKafka::PartitionMetadata {
public:
  MOCK_CONST_METHOD0(id, int32_t());
  MOCK_CONST_METHOD0(err, RdKafka::ErrorCode());
  MOCK_CONST_METHOD0(leader, int32_t());
  MOCK_CONST_METHOD0(replicas, const std::vector<int32_t> *());
  MOCK_CONST_METHOD0(isrs, const std::vector<int32_t> *());
};

class ConsumerStandIn : public Consumer {
public:
  ConsumerStandIn(BrokerSettings &Settings) : Consumer(Settings) {}
  using Consumer::KafkaConsumer;
};

class ConsumerTests : public ::testing::Test {
protected:
  void SetUp() override {
    Consumer = new MockKafkaConsumer;
    StandIn.KafkaConsumer.reset(Consumer);
  }

  MockKafkaConsumer *Consumer;
  BrokerSettings Settings;
  ConsumerStandIn StandIn{Settings};
};

TEST_F(ConsumerTests, pollReturnsConsumerMessageWithMessagePollStatus) {
  MockMessage * Message = new MockMessage;
  EXPECT_CALL(*Message, len()).Times(AtLeast(1)).WillOnce(Return(1));
  EXPECT_CALL(*Message, err())
      .Times(AtLeast(1))
      .WillOnce(Return(RdKafka::ErrorCode::ERR_NO_ERROR));
  std::string Payload{"test"};
  EXPECT_CALL(*Message, payload())
      .Times(AtLeast(1))
      .WillOnce(Return(reinterpret_cast<void *>(&Payload)));

  EXPECT_CALL(*Consumer, consume(_))
      .Times(Exactly(1))
      .WillOnce(Return(Message));
  EXPECT_CALL(*Consumer, close()).Times(Exactly(1));

  auto ConsumedMessage = StandIn.poll();
  ASSERT_EQ(ConsumedMessage->getStatus(), PollStatus::Message);
}

TEST_F(
    ConsumerTests,
    pollReturnsConsumerMessageWithEmptyPollStatusIfKafkaErrorMessageIsEmpty) {
  MockMessage * Message = new MockMessage;
  EXPECT_CALL(*Message, len()).Times(AtLeast(1)).WillOnce(Return(0));

  EXPECT_CALL(*Message, err())
      .Times(AtLeast(1))
      .WillOnce(Return(RdKafka::ErrorCode::ERR_NO_ERROR));
  EXPECT_CALL(*Consumer, consume(_))
      .Times(Exactly(1))
      .WillOnce(Return(Message));
  EXPECT_CALL(*Consumer, close()).Times(Exactly(1));

  auto ConsumedMessage = StandIn.poll();
  ASSERT_EQ(ConsumedMessage->getStatus(), PollStatus::Empty);
}

TEST_F(ConsumerTests,
       pollReturnsConsumerMessageWithEmptyPollStatusIfEndofPartition) {
  MockMessage * Message = new MockMessage;
  EXPECT_CALL(*Message, err())
      .Times(AtLeast(1))
      .WillOnce(Return(RdKafka::ErrorCode::ERR__PARTITION_EOF));
  EXPECT_CALL(*Consumer, consume(_))
      .Times(Exactly(1))
      .WillOnce(Return(Message));
  EXPECT_CALL(*Consumer, close()).Times(Exactly(1));

  auto ConsumedMessage = StandIn.poll();
  ASSERT_EQ(ConsumedMessage->getStatus(), PollStatus::EndOfPartition);
}

TEST_F(ConsumerTests,
       pollReturnsConsumerMessageWithErrorPollStatusIfUnknownOrUnexpected) {
  MockMessage * Message = new MockMessage;
  EXPECT_CALL(*Message, err())
      .Times(AtLeast(1))
      .WillOnce(Return(RdKafka::ErrorCode::ERR__BAD_MSG));
  EXPECT_CALL(*Consumer, consume(_))
      .Times(Exactly(1))
      .WillOnce(Return(Message));
  EXPECT_CALL(*Consumer, close()).Times(Exactly(1));

  auto ConsumedMessage = StandIn.poll();
  ASSERT_EQ(ConsumedMessage->getStatus(), PollStatus::Error);
}

TEST_F(ConsumerTests, getTopicPartitionNumbersThrowsErrorIfTopicsEmpty) {
  MockMetadata * Metadata = new MockMetadata;
  auto TopicVector = RdKafka::Metadata::TopicMetadataVector{};
  EXPECT_CALL(*Consumer, metadata(_, _, _, _))
      .Times(AtLeast(1))
      .WillOnce(DoAll(SetArgPointee<2>(Metadata),
                      Return(RdKafka::ErrorCode::ERR_NO_ERROR)));
  EXPECT_CALL(*Metadata, topics())
      .Times(Exactly(1))
      .WillOnce(Return(&TopicVector));
  EXPECT_CALL(*Consumer, close()).Times(Exactly(1));

  EXPECT_THROW(StandIn.addTopic("something"), std::runtime_error);
}

TEST_F(ConsumerTests, getTopicPartitionNumbersThrowsErrorIfTopicDoesntExist) {
  MockMetadata * Metadata = new MockMetadata;
  auto TopicVector = RdKafka::Metadata::TopicMetadataVector{
      new MockTopicMetadata("not_something")};
  EXPECT_CALL(*Consumer, metadata(_, _, _, _))
      .Times(AtLeast(1))
      .WillOnce(DoAll(SetArgPointee<2>(Metadata),
                      Return(RdKafka::ErrorCode::ERR_NO_ERROR)));
  EXPECT_CALL(*Metadata, topics())
      .Times(Exactly(1))
      .WillOnce(Return(&TopicVector));
  EXPECT_CALL(*Consumer, close()).Times(Exactly(1));
  EXPECT_THROW(StandIn.addTopic("something"), std::runtime_error);
}

TEST_F(ConsumerTests,
       getTopicPartitionNumbersReturnsPartitionNumbersIfTopicDoesExist) {
  MockMetadata * Metadata = new MockMetadata;
  auto TopicMetadata = new MockTopicMetadata("something");
  auto TopicVector = RdKafka::Metadata::TopicMetadataVector{TopicMetadata};
  auto PartitionMetadata = new MockPartitionMetadata;
  auto PartitionMetadataVector =
      RdKafka::TopicMetadata::PartitionMetadataVector{PartitionMetadata};
  EXPECT_CALL(*Consumer, metadata(_, _, _, _))
      .Times(AtLeast(1))
      .WillOnce(DoAll(SetArgPointee<2>(Metadata),
                      Return(RdKafka::ErrorCode::ERR_NO_ERROR)));
  EXPECT_CALL(*Metadata, topics())
      .Times(Exactly(1))
      .WillOnce(Return(&TopicVector));
  EXPECT_CALL(*TopicMetadata, partitions())
      .Times(AtLeast(1))
      .WillOnce(Return(&PartitionMetadataVector));
  EXPECT_CALL(*PartitionMetadata, id()).Times(AtLeast(1)).WillOnce(Return(1));
  EXPECT_CALL(*Consumer, close()).Times(Exactly(1));
  EXPECT_CALL(*Consumer, assign(_))
      .Times(Exactly(1))
      .WillOnce(Return(RdKafka::ERR_NO_ERROR));
  ASSERT_NO_THROW(StandIn.addTopic("something"));
  // Clean up pointers
  delete TopicMetadata;
  delete PartitionMetadata;
}
