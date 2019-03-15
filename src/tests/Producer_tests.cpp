#include "../KafkaW/Producer.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <librdkafka/rdkafkacpp.h>
using ::testing::AtLeast;
using ::testing::Exactly;
using ::testing::Return;
using ::testing::_;

using namespace KafkaW;

class ProducerTests : public ::testing::Test {
protected:
  void SetUp() override {}
};

class ProducerStandIn : public Producer {
public:
  explicit ProducerStandIn(const BrokerSettings &Settings)
      : Producer(Settings) {}
  using Producer::ProducerID;
  using Producer::ProducerPtr;
};

class MockProducer : public RdKafka::Producer {
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
  MOCK_METHOD2(create, RdKafka::Producer *(RdKafka::Conf *, std::string));
  MOCK_METHOD7(produce,
               RdKafka::ErrorCode(RdKafka::Topic *, int32_t, int, void *,
                                  size_t, const std::string *, void *));
  MOCK_METHOD8(produce,
               RdKafka::ErrorCode(RdKafka::Topic *, int32_t, int, void *,
                                  size_t, const void *, size_t, void *));
  MOCK_METHOD9(produce, RdKafka::ErrorCode(const std::string, int32_t, int,
                                           void *, size_t, const void *, size_t,
                                           int64_t, void *));
  MOCK_METHOD5(produce, RdKafka::ErrorCode(RdKafka::Topic *, int32_t,
                                           const std::vector<char> *,
                                           const std::vector<char> *, void *));
  MOCK_METHOD1(flush, RdKafka::ErrorCode(int));
};

class FakeTopic : public RdKafka::Topic {
public:
  FakeTopic() = default;
  ~FakeTopic() override = default;
  const std::string name() const override {};
  bool partition_available(int32_t partition) const override { return true; };
  RdKafka::ErrorCode offset_store(int32_t partition, int64_t offset) override{};
  struct rd_kafka_topic_s *c_ptr() override{};
};

TEST_F(ProducerTests, creatingForwarderIncrementsForwarderCounter) {
  BrokerSettings Settings{};
  ProducerStandIn Producer1(Settings);
  ProducerStandIn Producer2(Settings);
  ASSERT_EQ(-1, Producer1.ProducerID - Producer2.ProducerID);
}

TEST_F(ProducerTests, callPollTest) {
  BrokerSettings Settings{};
  ProducerStandIn Producer1(Settings);
  Producer1.ProducerPtr.reset(new MockProducer);
  EXPECT_CALL(*dynamic_cast<MockProducer *>(Producer1.ProducerPtr.get()),
              poll(_))
      .Times(AtLeast(1));

  EXPECT_CALL(*dynamic_cast<MockProducer *>(Producer1.ProducerPtr.get()),
              outq_len())
      .Times(AtLeast(1));

  Producer1.poll();
}

TEST_F(ProducerTests, produceReturnsNoErrorCodeIfMessageProduced) {
  BrokerSettings Settings{};
  ProducerStandIn Producer1(Settings);
  Producer1.ProducerPtr.reset(new MockProducer);
  EXPECT_CALL(*dynamic_cast<MockProducer *>(Producer1.ProducerPtr.get()),
              produce(_, _, _, _, _, _, _, _))
      .Times(Exactly(1))
      .WillOnce(Return(RdKafka::ERR_NO_ERROR));
  ASSERT_EQ(
      Producer1.produce(new FakeTopic, 0, 0, nullptr, 0, nullptr, 0, nullptr),
      RdKafka::ErrorCode::ERR_NO_ERROR);
}

TEST_F(ProducerTests, produceReturnsErrorCodeIfMessageNotProduced) {
  BrokerSettings Settings{};
  ProducerStandIn Producer1(Settings);
  Producer1.ProducerPtr.reset(new MockProducer);
  EXPECT_CALL(*dynamic_cast<MockProducer *>(Producer1.ProducerPtr.get()),
              produce(_, _, _, _, _, _, _, _))
      .Times(Exactly(1))
      .WillOnce(Return(RdKafka::ERR__BAD_MSG));
  ASSERT_EQ(
      Producer1.produce(new FakeTopic, 0, 0, nullptr, 0, nullptr, 0, nullptr),
      RdKafka::ErrorCode::ERR__BAD_MSG);
}
