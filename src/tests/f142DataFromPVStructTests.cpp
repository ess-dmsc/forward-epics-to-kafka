#include "schemas/f142/DataFromPVStruct.h"
#include <fmt/core.h>
#include <gtest/gtest.h>
#include <pv/nt.h>
#include <pv/pvTimeStamp.h>

epics::pvData::PVStructure::shared_pointer CreateTestPVStructWithAlarm(std::string const &AlarmMessage) {
  auto Builder = epics::nt::NTScalar::createBuilder();
  auto PVStruct =
      Builder->value(epics::pvData::pvDouble)->addTimeStamp()->create();
  auto ValueField = PVStruct->getValue<epics::pvData::PVDouble>();
  ValueField->put(3.14);
  epics::pvData::TimeStamp TS;
  TS.getCurrent();
  epics::pvData::PVTimeStamp pvTS;
  PVStruct->attachTimeStamp(pvTS);

  fmt::print("{}\n", AlarmMessage);
  auto AlarmStruct = PVStruct->getAlarm();

  // Make one of these:
  auto AlarmBuilder = epics::pvData::Alarm();
  AlarmBuilder.setMessage(AlarmMessage);
  AlarmBuilder.setStatus();
  AlarmBuilder.setSeverity();

  // Convert it to one of these:
  pvAlarm ConstructedPVAlarm;

  // Then attach it to the PVStruct
  PVStruct->attachAlarm(ConstructedPVAlarm);

  return PVStruct->getPVStructure();
}

TEST(f142Test, some_test) {
  ASSERT_NO_THROW(CreateTestPVStructWithAlarm("HIHI_ALARM"));
}
