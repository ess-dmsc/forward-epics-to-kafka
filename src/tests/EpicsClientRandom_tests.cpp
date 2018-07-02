#include "../EpicsClient/EpicsClientRandom.h"
#include "EpicsPVUpdate.h"
#include "Streams.h"
#include <gtest/gtest.h>
#include <memory>

using namespace Forwarder;

TEST(EpicsClientRandomTest,
     calling_GeneratePVUpdate_results_in_a_PV_update_in_the_buffer) {
  // GIVEN an EpicsClient with a ring buffer
  auto RingBuffer = std::make_shared<
      moodycamel::ConcurrentQueue<std::shared_ptr<FlatBufs::EpicsPVUpdate>>>();
  ChannelInfo ChannelInformation{"", ""};
  auto EpicsClient =
      EpicsClient::EpicsClientRandom(ChannelInformation, RingBuffer);

  // WHEN we call generateFakePVUpdate once
  EpicsClient.generateFakePVUpdate();

  // THEN there will be a single EpicsPVUpdate in the ring buffer
  std::shared_ptr<FlatBufs::EpicsPVUpdate> FirstPV;
  ASSERT_TRUE(RingBuffer->try_dequeue(FirstPV));

  // this time expect failure as only one should have been created
  std::shared_ptr<FlatBufs::EpicsPVUpdate> SecondPV;
  ASSERT_FALSE(RingBuffer->try_dequeue(SecondPV));
}

TEST(EpicsClientRandomTest,
     calling_GeneratePVUpdate_results_in_different_PV_values) {
  // GIVEN an EpicsClient with a ring buffer
  auto RingBuffer = std::make_shared<
      moodycamel::ConcurrentQueue<std::shared_ptr<FlatBufs::EpicsPVUpdate>>>();
  ChannelInfo ChannelInformation{"", ""};
  auto EpicsClient =
      EpicsClient::EpicsClientRandom(ChannelInformation, RingBuffer);

  // WHEN we call generateFakePVUpdate twice
  EpicsClient.generateFakePVUpdate();
  EpicsClient.generateFakePVUpdate();

  // THEN there will be two EpicsPVUpdates in the ring buffer with different
  // values
  std::shared_ptr<FlatBufs::EpicsPVUpdate> FirstPV;
  ASSERT_TRUE(RingBuffer->try_dequeue(FirstPV));
  std::shared_ptr<FlatBufs::EpicsPVUpdate> SecondPV;
  ASSERT_TRUE(RingBuffer->try_dequeue(SecondPV));

  double FirstGeneratedPVValue =
      FirstPV->epics_pvstr->getSubField<epics::pvData::PVDouble>("value")
          ->get();
  double SecondGeneratedPVValue =
      SecondPV->epics_pvstr->getSubField<epics::pvData::PVDouble>("value")
          ->get();

  // We should be generating random values for the PVs, so this tests the values
  // we get are different
  ASSERT_GT(abs(FirstGeneratedPVValue - SecondGeneratedPVValue), 0.00001);
}
