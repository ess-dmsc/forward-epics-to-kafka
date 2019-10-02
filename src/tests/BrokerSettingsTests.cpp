// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

#include "ConfStandIn.h"
#include <KafkaW/BrokerSettings.h>
#include <gtest/gtest.h>
#include <librdkafka/rdkafkacpp.h>

using ::testing::Exactly;
using ::testing::_;
using ::testing::An;
using ::testing::Return;

using namespace KafkaW;

class BrokerSettingsTests : public ::testing::Test {};

TEST_F(BrokerSettingsTests, callingApplyCallsSetOnKafkaConfObject) {
  BrokerSettings Settings;
  ConfStandIn Conf;
  EXPECT_CALL(Conf, set(An<const std::string &>(), An<const std::string &>(),
                        An<std::string &>()))
      .Times(Exactly(Settings.KafkaConfiguration.size()));
  Settings.apply(&Conf);
}

TEST_F(BrokerSettingsTests, callingApplyThrows) {
  BrokerSettings Settings;
  ConfStandIn Conf;
  EXPECT_CALL(Conf, set(An<const std::string &>(), An<const std::string &>(),
                        An<std::string &>()))
      .Times(Exactly(1))
      .WillRepeatedly(Return(RdKafka::Conf::ConfResult::CONF_INVALID));
  EXPECT_THROW(Settings.apply(&Conf), std::runtime_error);
}
