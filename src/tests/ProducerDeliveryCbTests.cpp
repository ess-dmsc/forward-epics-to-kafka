// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

#include "MockMessage.h"
#include <KafkaW/Producer.h>
#include <gtest/gtest.h>
#include <librdkafka/rdkafkacpp.h>

using ::testing::AtLeast;
using ::testing::Exactly;
using ::testing::Return;
using namespace KafkaW;

class ProducerDeliveryCbTests : public ::testing::Test {
protected:
  void SetUp() override {}
};

struct ProducerMessageStandIn : ProducerMessage {
  explicit ProducerMessageStandIn(std::function<void()> DestructorFunction)
      : Fun(DestructorFunction) {}
  ~ProducerMessageStandIn() override { Fun(); }
  std::function<void()> Fun;
};

TEST_F(ProducerDeliveryCbTests,
       deliveryCbIncrementsProduceStatsOnSuccessAndReleasesOpaquePointer) {
  bool Called = false;

  ProducerMessageStandIn *FakeMessage =
      new ProducerMessageStandIn([&Called]() { Called = true; });
  MockMessage Message;
  RdKafka::Message *TempPtr = reinterpret_cast<RdKafka::Message *>(&Message);
  EXPECT_CALL(Message, err())
      .Times(Exactly(1))
      .WillOnce(Return(RdKafka::ErrorCode::ERR_NO_ERROR));
  EXPECT_CALL(Message, msg_opaque())
      .Times(Exactly(1))
      .WillOnce(Return(reinterpret_cast<void *>(FakeMessage)));
  ProducerStats Stats;
  ProducerDeliveryCb Callback(Stats);
  Callback.dr_cb(*TempPtr);
  ASSERT_TRUE(Called);
  EXPECT_EQ(Stats.produce_cb, 1);
  EXPECT_EQ(Stats.produce_cb_fail, 0);
}

TEST_F(ProducerDeliveryCbTests,
       deliveryCbIncrementsProduceFailStatsOnFailureAndReleasesOpaquePointer) {
  bool Called = false;

  ProducerMessageStandIn *FakeMessage =
      new ProducerMessageStandIn([&Called]() { Called = true; });
  MockMessage Message;
  RdKafka::Message *TempPtr = reinterpret_cast<RdKafka::Message *>(&Message);
  EXPECT_CALL(Message, err())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(RdKafka::ErrorCode::ERR__BAD_MSG));
  EXPECT_CALL(Message, msg_opaque())
      .Times(Exactly(1))
      .WillOnce(Return(reinterpret_cast<void *>(FakeMessage)));
  ProducerStats Stats;
  ProducerDeliveryCb Callback(Stats);
  Callback.dr_cb(*TempPtr);
  ASSERT_TRUE(Called);
  EXPECT_EQ(Stats.produce_cb, 0);
  EXPECT_EQ(Stats.produce_cb_fail, 1);
}
