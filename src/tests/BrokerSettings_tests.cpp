#include "../KafkaW/BrokerSettings.h"
#include "ConfStandIn.h"
#include <gtest/gtest.h>
#include <librdkafka/rdkafkacpp.h>

using ::testing::Exactly;
using ::testing::_;
using ::testing::An;
using namespace KafkaW;

class BrokerSettingsTests : public ::testing::Test {
protected:
  void SetUp() override {}
};

TEST_F(BrokerSettingsTests, testSomething) {
  BrokerSettings Settings;
  ConfStandIn Conf;
  EXPECT_CALL(Conf, set(An<const std::string &>(), An<const std::string &>(),
                        An<std::string &>()))
      .Times(Exactly(Settings.KafkaConfiguration.size()));
  Settings.apply(&Conf);
}
