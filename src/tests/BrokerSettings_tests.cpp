#include <gtest/gtest.h>
#include "../KafkaW/BrokerSettings.h"
#include "ConfStandIn.h"
#include <librdkafka/rdkafkacpp.h>

using ::testing::AtLeast;
using namespace KafkaW;

class BrokerSettingsTests : public ::testing::Test {
protected:
    void SetUp() override {}

};

TEST_F(BrokerSettingsTests, testSomething) {
    BrokerSettings Settings;
    ConfStandIn Conf;
    Settings.apply(&Conf)
}
