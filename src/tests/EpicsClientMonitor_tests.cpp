#include "../EpicsClient/EpicsClientMonitor.h"
#include <gtest/gtest.h>
#include <helper.h>

using namespace Forwarder;

TEST(EpicsClientMonitorTest, TestValueIsCachedWhenEmitIsCalledFirstTime) {
  ChannelInfo ChannelInfo;
  ChannelInfo.channel_name = "SIM:Spd";
  ChannelInfo.provider_type = "ca";

  auto UpdatePtr = std::make_shared<FlatBufs::EpicsPVUpdate>();

  auto PVUpdateRing = std::make_shared<
      moodycamel::ConcurrentQueue<std::shared_ptr<FlatBufs::EpicsPVUpdate>>>();
  EpicsClient::EpicsClientMonitor Client(ChannelInfo, PVUpdateRing);

  // First emit the update
  Client.emit(UpdatePtr);

  // Then call the callback function to emit the cached value - it should exist
  // now as emit has been called
  Client.emitCachedValue();

  auto FirstValue = std::shared_ptr<FlatBufs::EpicsPVUpdate>();
  ASSERT_TRUE(PVUpdateRing->try_dequeue(FirstValue));

  // There should be a second update in the buffer as the cached value should
  // have been emitted
  auto SecondValue = std::shared_ptr<FlatBufs::EpicsPVUpdate>();
  ASSERT_TRUE(PVUpdateRing->try_dequeue(SecondValue));

  // There shouldn't be any values left in the ring buffer as just the update
  // and the cached value have been emitted.
  auto ThirdValue = std::shared_ptr<FlatBufs::EpicsPVUpdate>();
  ASSERT_FALSE(PVUpdateRing->try_dequeue(ThirdValue));
}

TEST(EpicsClientMonitorTest,
     TestCachedValueIsNotPushedWhenEmitIsCalledWithoutEmitCachedValue) {
  ChannelInfo ChannelInfo;
  ChannelInfo.channel_name = "SIM:Spd";
  ChannelInfo.provider_type = "ca";

  auto UpdatePtr = std::make_shared<FlatBufs::EpicsPVUpdate>();

  auto PVUpdateRing = std::make_shared<
      moodycamel::ConcurrentQueue<std::shared_ptr<FlatBufs::EpicsPVUpdate>>>();
  EpicsClient::EpicsClientMonitor Client(ChannelInfo, PVUpdateRing);

  // First emit the update
  Client.emit(UpdatePtr);

  auto FirstValue = std::shared_ptr<FlatBufs::EpicsPVUpdate>();
  ASSERT_TRUE(PVUpdateRing->try_dequeue(FirstValue));

  // There should not be a second update in the buffer as the cached value
  // should not have been emitted
  auto SecondValue = std::shared_ptr<FlatBufs::EpicsPVUpdate>();
  ASSERT_FALSE(PVUpdateRing->try_dequeue(SecondValue));
}

TEST(EpicsClientMonitorTest,
     TestCachedValueIsNotUpdatedWhenEmitWithoutCachingIsUsed) {
  ChannelInfo ChannelInfo;
  ChannelInfo.channel_name = "SIM:Spd";
  ChannelInfo.provider_type = "ca";

  auto UpdatePtr = std::make_shared<FlatBufs::EpicsPVUpdate>();

  auto PVUpdateRing = std::make_shared<
      moodycamel::ConcurrentQueue<std::shared_ptr<FlatBufs::EpicsPVUpdate>>>();
  EpicsClient::EpicsClientMonitor Client(ChannelInfo, PVUpdateRing);

  // First emit the update
  Client.emitWithoutCaching(UpdatePtr);

  // Do not throw any exceptions when using a nullptr as the cached update
  ASSERT_NO_THROW(Client.emitCachedValue());

  auto FirstValue = std::shared_ptr<FlatBufs::EpicsPVUpdate>();
  ASSERT_TRUE(PVUpdateRing->try_dequeue(FirstValue));

  // Should be empty as no cached update should have been pushed.
  auto SecondValue = std::shared_ptr<FlatBufs::EpicsPVUpdate>();
  ASSERT_FALSE(PVUpdateRing->try_dequeue(SecondValue));
}

TEST(EpicsClientMonitorTest, TestCachedValueIsNotPushedWhenNoValueIsEmitted) {
  ChannelInfo ChannelInfo;
  ChannelInfo.channel_name = "SIM:Spd";
  ChannelInfo.provider_type = "ca";

  auto UpdatePtr = std::make_shared<FlatBufs::EpicsPVUpdate>();

  auto PVUpdateRing = std::make_shared<
      moodycamel::ConcurrentQueue<std::shared_ptr<FlatBufs::EpicsPVUpdate>>>();
  EpicsClient::EpicsClientMonitor Client(ChannelInfo, PVUpdateRing);

  // Do not throw any exceptions when using a nullptr as the cached update
  ASSERT_NO_THROW(Client.emitCachedValue());

  // Should be empty as no cached update should have been pushed.
  auto FirstValue = std::shared_ptr<FlatBufs::EpicsPVUpdate>();
  ASSERT_FALSE(PVUpdateRing->try_dequeue(FirstValue));
}
