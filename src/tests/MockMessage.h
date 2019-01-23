#pragma once
#include <gmock/gmock.h>
#include <librdkafka/rdkafkacpp.h>


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
