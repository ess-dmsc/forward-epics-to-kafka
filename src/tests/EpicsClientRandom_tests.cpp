#include <gtest/gtest.h>
#include <memory>
#include "Ring.h"
#include "EpicsPVUpdate.h"
#include "../EpicsClient/EpicsClientRandom.h"

using namespace Forwarder;

TEST(EpicsClientRandomTests, CallingGeneratePVUpdateResultsInAPVUpdateInTheBuffer) {
  auto RingBuffer = std::make_shared<Ring<std::unique_ptr<FlatBufs::EpicsPVUpdate>>>();
  auto EpicsClient = EpicsClient::EpicsClientRandom(RingBuffer);

  EpicsClient.generateFakePVUpdate();
  std::pair<int, std::unique_ptr<FlatBufs::EpicsPVUpdate>> GeneratedPV = RingBuffer->pop();
  ASSERT_EQ(GeneratedPV.first, 0);  // 0 indicates success
}
