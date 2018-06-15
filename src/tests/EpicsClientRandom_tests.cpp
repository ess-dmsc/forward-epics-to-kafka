#include "../EpicsClient/EpicsClientRandom.h"
#include "EpicsPVUpdate.h"
#include "Ring.h"
#include "Streams.h"
#include <gtest/gtest.h>
#include <memory>

using namespace Forwarder;

TEST(EpicsClientRandomTests,
     CallingGeneratePVUpdateResultsInAPVUpdateInTheBuffer) {
  // GIVEN an EpicsClient with a ring buffer
  auto RingBuffer =
      std::make_shared<Ring<std::unique_ptr<FlatBufs::EpicsPVUpdate>>>();
  ChannelInfo ChannelInformation{"", ""};
  auto EpicsClient =
      EpicsClient::EpicsClientRandom(ChannelInformation, RingBuffer);

  // WHEN we call generateFakePVUpdate once
  EpicsClient.generateFakePVUpdate();

  // THEN there will be a single EpicsPVUpdate in the ring buffer
  std::pair<int, std::unique_ptr<FlatBufs::EpicsPVUpdate>> GeneratedPV =
      RingBuffer->pop();
  ASSERT_EQ(GeneratedPV.first, 0); // 0 indicates success
  std::pair<int, std::unique_ptr<FlatBufs::EpicsPVUpdate>>
      PossibleSecondGeneratedPV = RingBuffer->pop();
  ASSERT_EQ(PossibleSecondGeneratedPV.first,
            1); // this time expect failure as only one should have been created
}

TEST(EpicsClientRandomTests,
     CallingGeneratePVUpdateResultsInDifferentPVValues) {
  // GIVEN an EpicsClient with a ring buffer
  auto RingBuffer =
      std::make_shared<Ring<std::unique_ptr<FlatBufs::EpicsPVUpdate>>>();
  ChannelInfo ChannelInformation{"", ""};
  auto EpicsClient =
      EpicsClient::EpicsClientRandom(ChannelInformation, RingBuffer);

  // WHEN we call generateFakePVUpdate once
  EpicsClient.generateFakePVUpdate();
  EpicsClient.generateFakePVUpdate();

  // THEN there will be two EpicsPVUpdates in the ring buffer with different
  // values
  std::pair<int, std::unique_ptr<FlatBufs::EpicsPVUpdate>> FirstGeneratedPV =
      RingBuffer->pop();
  ASSERT_EQ(FirstGeneratedPV.first, 0); // 0 indicates success
  std::pair<int, std::unique_ptr<FlatBufs::EpicsPVUpdate>> SecondGeneratedPV =
      RingBuffer->pop();
  ASSERT_EQ(SecondGeneratedPV.first, 0); // expect a second PVUpdate

  double FirstGeneratedPVValue =
      FirstGeneratedPV.second->epics_pvstr
          ->getSubField<epics::pvData::PVDouble>("value")
          ->get();
  double SecondGeneratedPVValue =
      SecondGeneratedPV.second->epics_pvstr
          ->getSubField<epics::pvData::PVDouble>("value")
          ->get();

  // We should be generating random values for the PVs, so this tests the values
  // we get are different
  ASSERT_GT(abs(FirstGeneratedPVValue - SecondGeneratedPVValue), 0.00001);
}
