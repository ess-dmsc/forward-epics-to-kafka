#pragma once
#include <librdkafka/rdkafkacpp.h>
#include <gmock/gmock.h>

class ConfStandIn : public RdKafka::Conf {
public:
    MOCK_METHOD3(set, RdKafka::Conf::ConfResult(const std::string&, const std::string&, std::string&));
};
