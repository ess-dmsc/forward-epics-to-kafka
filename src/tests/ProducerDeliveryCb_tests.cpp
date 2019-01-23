#include "../KafkaW/Producer.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <librdkafka/rdkafkacpp.h>
using ::testing::Exactly;
using ::testing::AtLeast;
using ::testing::Return;
using namespace KafkaW;

class ProducerDeliveryCbTests : public ::testing::Test {
protected:
    void SetUp() override {}
};

class MockMessage : public RdKafka::Message {
public:
    MOCK_CONST_METHOD0(errstr, std::string());
    MOCK_CONST_METHOD0(err, RdKafka::ErrorCode());
    MOCK_CONST_METHOD0(topic, RdKafka::Topic*());
    MOCK_CONST_METHOD0(topic_name, std::string());
    MOCK_CONST_METHOD0(partition, int32_t());
    MOCK_CONST_METHOD0(payload, void*());
    MOCK_CONST_METHOD0(len, size_t());
    MOCK_CONST_METHOD0(key, const std::string*());
    MOCK_CONST_METHOD0(key_pointer, const void*());
    MOCK_CONST_METHOD0(key_len, size_t());
    MOCK_CONST_METHOD0(offset, int64_t());
    MOCK_CONST_METHOD0(timestamp, RdKafka::MessageTimestamp());
    MOCK_CONST_METHOD0(msg_opaque, void*());
    MOCK_CONST_METHOD0(latency, int64_t());
    MOCK_METHOD0(c_ptr, rd_kafka_message_s*());
};

struct ProducerMessageStandIn : ProducerMessage {
    ProducerMessageStandIn(std::function<void()> DestructorFunction) : Fun(DestructorFunction) {}
    ~ProducerMessageStandIn() override {Fun();}
    std::function<void()> Fun;
};


TEST_F(ProducerDeliveryCbTests, deliveryCbIncrementsProduceStatsOnSuccessAndReleasesOpaquePointer){
    bool Called = false;

    ProducerMessageStandIn* FakeMessage = new ProducerMessageStandIn([&Called](){Called=true;});
    MockMessage Message;
    RdKafka::Message* TempPtr = reinterpret_cast<RdKafka::Message*>(&Message);
    EXPECT_CALL(Message, err()).Times(Exactly(1)).WillOnce(Return(RdKafka::ErrorCode::ERR_NO_ERROR));
    EXPECT_CALL(Message, msg_opaque()).Times(Exactly(1)).WillOnce(Return(reinterpret_cast<void*>(FakeMessage)));
    ProducerStats Stats;
    ProducerDeliveryCb Callback(Stats);
    Callback.dr_cb(*TempPtr);
    ASSERT_TRUE(Called);
    EXPECT_EQ(Stats.produce_cb, 1);
    EXPECT_EQ(Stats.produce_cb_fail, 0);
}

TEST_F(ProducerDeliveryCbTests, deliveryCbIncrementsProduceFailStatsOnFailureAndReleasesOpaquePointer){
    bool Called = false;

    ProducerMessageStandIn* FakeMessage = new ProducerMessageStandIn([&Called](){Called=true;});
    MockMessage Message;
    RdKafka::Message* TempPtr = reinterpret_cast<RdKafka::Message*>(&Message);
    EXPECT_CALL(Message, err()).Times(AtLeast(1)).WillRepeatedly(Return(RdKafka::ErrorCode::ERR__BAD_MSG));
    EXPECT_CALL(Message, msg_opaque()).Times(Exactly(1)).WillOnce(Return(reinterpret_cast<void*>(FakeMessage)));
    ProducerStats Stats;
    ProducerDeliveryCb Callback(Stats);
    Callback.dr_cb(*TempPtr);
    ASSERT_TRUE(Called);
    EXPECT_EQ(Stats.produce_cb, 0);
    EXPECT_EQ(Stats.produce_cb_fail, 1);
}