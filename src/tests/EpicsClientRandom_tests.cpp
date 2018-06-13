#include <gtest/gtest.h>
#include <memory>
#include "Ring.h"
#include "EpicsPVUpdate.h"
#include "../EpicsClient/EpicsClientRandom.h"

using namespace Forwarder;

TEST(EpicsClientRandomTests, CreatingAnEpicsClientRandomWithARingBufferIsSuccessful) {
  auto RingBuffer = std::make_shared<Ring<std::unique_ptr<FlatBufs::EpicsPVUpdate>>>();
  auto EpicsClient = EpicsClient::EpicsClientRandom(RingBuffer);
}
