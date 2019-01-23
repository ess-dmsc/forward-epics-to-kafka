#include "../KafkaW/Consumer.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <librdkafka/rdkafkacpp.h>
using ::testing::AtLeast;
using ::testing::Exactly;
using ::testing::Return;
using ::testing::_;

using namespace KafkaW;

class ConsumerTests : public ::testing::Test {
protected:
    void SetUp() override {}
};

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
    MOCK_METHOD1(subscribe, RdKafka::ErrorCode(const std::vector<std::string>&));
    MOCK_METHOD0(unsubscribe, RdKafka::ErrorCode());
    MOCK_METHOD1(assign, RdKafka::ErrorCode(const std::vector<RdKafka::TopicPartition*>&));
    MOCK_METHOD0(unassign, RdKafka::ErrorCode());
    MOCK_METHOD1(consume, RdKafka::Message*(int));
    MOCK_METHOD0(commitSync, RdKafka::ErrorCode());
    MOCK_METHOD0(commitAsync, RdKafka::ErrorCode());
    MOCK_METHOD1(commitSync, RdKafka::ErrorCode(RdKafka::Message*));
    MOCK_METHOD1(commitAsync, RdKafka::ErrorCode(RdKafka::Message*));
    MOCK_METHOD1(commitSync, RdKafka::ErrorCode(std::vector<RdKafka::TopicPartition*>&));
    MOCK_METHOD1(commitAsync, RdKafka::ErrorCode(const std::vector<RdKafka::TopicPartition*>&));
    MOCK_METHOD1(commitSync, RdKafka::ErrorCode(RdKafka::OffsetCommitCb*));
    MOCK_METHOD2(commitSync, RdKafka::ErrorCode(std::vector<RdKafka::TopicPartition*>&, RdKafka::OffsetCommitCb*));
    MOCK_METHOD2(committed, RdKafka::ErrorCode(std::vector<RdKafka::TopicPartition*>&, int));
    MOCK_METHOD1(position, RdKafka::ErrorCode(std::vector<RdKafka::TopicPartition*>&));
    MOCK_METHOD0(close, RdKafka::ErrorCode());
    MOCK_METHOD2(seek, RdKafka::ErrorCode(const RdKafka::TopicPartition&, int));
    MOCK_METHOD1(offsets_store, RdKafka::ErrorCode(std::vector<RdKafka::TopicPartition*>&));
    MOCK_METHOD1(assignment, RdKafka::ErrorCode(std::vector<RdKafka::TopicPartition*>&));
    MOCK_METHOD1(subscription, RdKafka::ErrorCode(std::vector<std::string>&));
};

class ConsumerStandIn : public Consumer {
public:
    ConsumerStandIn(BrokerSettings& Settings) : Consumer(Settings) {

    }
    using Consumer::KafkaConsumer;
    std::vector<int32_t> getTopicPartitionNumbers(const std::string& Topic) override {
        return {};
    }
};


TEST_F(ConsumerTests, addTopicAddsTopicObject) {
    auto Consumer = new MockKafkaConsumer;
    BrokerSettings Settings;
    ConsumerStandIn StandIn(Settings);
    StandIn.KafkaConsumer.reset(Consumer);
    EXPECT_CALL(*Consumer, assign(_)).Times(Exactly(1)).WillOnce(Return(RdKafka::ErrorCode::ERR_NO_ERROR));
    StandIn.addTopic("something");
}

TEST_F(ConsumerTests, addTopicThrowsWhenFailsToAssign) {
    auto Consumer = new MockKafkaConsumer;
    BrokerSettings Settings;
    ConsumerStandIn StandIn(Settings);
    StandIn.KafkaConsumer.reset(Consumer);
    EXPECT_CALL(*Consumer, assign(_)).Times(Exactly(1)).WillOnce(Return(RdKafka::ErrorCode::ERR__ASSIGN_PARTITIONS));
    EXPECT_THROW(StandIn.addTopic("something"), std::runtime_error);
}
