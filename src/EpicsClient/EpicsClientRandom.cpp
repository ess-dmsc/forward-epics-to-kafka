// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

#include "EpicsClientRandom.h"
#include <memory>
#include <pv/pvData.h>

namespace Forwarder {
namespace EpicsClient {

void EpicsClientRandom::emit(std::shared_ptr<FlatBufs::EpicsPVUpdate> Update) {
  EmitQueue->enqueue(Update);
}

void EpicsClientRandom::generateFakePVUpdate() {
  auto FakePVUpdate = std::make_unique<FlatBufs::EpicsPVUpdate>();
  FakePVUpdate->epics_pvstr = epics::pvData::PVStructure::shared_pointer(
      createFakePVStructure(UniformDistribution(RandomEngine)));
  FakePVUpdate->channel = ChannelInformation.channel_name;

  emit(std::move(FakePVUpdate));
}

epics::pvData::PVStructurePtr
EpicsClientRandom::createFakePVStructure(double Value) const {
  auto FieldCreator = epics::pvData::getFieldCreate();
  auto PVDataCreator = epics::pvData::getPVDataCreate();
  auto PVFieldBuilder = FieldCreator->createFieldBuilder();
  auto PVTimestampFieldBuilder = FieldCreator->createFieldBuilder();

  PVTimestampFieldBuilder->add("secondsPastEpoch", epics::pvData::pvLong);
  PVTimestampFieldBuilder->add("nanoseconds", epics::pvData::pvInt);
  auto TimestampStructure = PVTimestampFieldBuilder->createStructure();

  PVFieldBuilder->add("value", epics::pvData::pvDouble);
  PVFieldBuilder->add("timeStamp", TimestampStructure);
  auto Structure = PVFieldBuilder->createStructure();
  auto FakePVStructure = PVDataCreator->createPVStructure(Structure);
  epics::pvData::PVDoublePtr FakePVDouble =
      FakePVStructure->getSubField<epics::pvData::PVDouble>("value");
  FakePVDouble->put(Value);

  // Populate timestamp
  auto currentTimestamp = getCurrentTimestamp();
  auto currentTimestampSeconds = currentTimestamp / 1000000000L;
  auto currentTimestampNanosecondsComponent =
      currentTimestamp - (currentTimestampSeconds * 1000000000L);
  auto Timestamp =
      FakePVStructure->getSubField<epics::pvData::PVStructure>("timeStamp");
  auto TimestampSeconds =
      Timestamp->getSubField<epics::pvData::PVScalarValue<int64_t>>(
          "secondsPastEpoch");
  TimestampSeconds->put(static_cast<int64_t>(currentTimestampSeconds));
  auto TimestampNanoseconds =
      Timestamp->getSubField<epics::pvData::PVScalarValue<int32_t>>(
          "nanoseconds");
  TimestampNanoseconds->put(
      static_cast<int32_t>(currentTimestampNanosecondsComponent));

  return FakePVStructure;
}

uint64_t EpicsClientRandom::getCurrentTimestamp() const {
  uint64_t CurrentTimestamp = static_cast<uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count());
  return CurrentTimestamp;
}
} // namespace EpicsClient
} // namespace Forwarder
