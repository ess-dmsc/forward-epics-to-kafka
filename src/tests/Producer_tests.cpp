#include "../KafkaW/Producer.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <librdkafka/rdkafkacpp.h>
using ::testing::AtLeast;
using namespace KafkaW;

class ProducerTests : public ::testing::Test {
protected:
  void SetUp() override {}
};

class ProducerStandIn : public Producer {
public:
  ProducerStandIn(BrokerSettings Settings) : Producer(Settings) {}
  using Producer::ProducerID;
  using Producer::ProducerPtr;
};

TEST_F(ProducerTests, creatingForwarderIncrementsForwarderCounter) {
  BrokerSettings Settings{};
  ProducerStandIn Producer1(Settings);
  ProducerStandIn Producer2(Settings);
  ASSERT_EQ(-1, Producer1.ProducerID - Producer2.ProducerID);
}

class MockHandle : public RdKafka::Handle {
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
};

TEST_F(ProducerTests, callPollTest) {
  BrokerSettings Settings{};
  ProducerStandIn Producer1(Settings);
  Producer1.ProducerPtr.reset(new MockHandle);
  EXPECT_CALL(*dynamic_cast<MockHandle *>(Producer1.ProducerPtr.get()),
              poll(::testing::_))
      .Times(AtLeast(1));

  EXPECT_CALL(*dynamic_cast<MockHandle *>(Producer1.ProducerPtr.get()),
              outq_len())
      .Times(AtLeast(1));

  Producer1.poll();
}
