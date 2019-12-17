#include "schemas/f142/DataFromPVStruct.h"
#include <gtest/gtest.h>
#include <pv/pvAlarm.h>
#include <pv/nt.h>
#include <pv/pvTimeStamp.h>

epics::pvData::PVStructure::shared_pointer CreateTestPVStructWithAlarm(std::string const &AlarmMessage) {
  auto Builder = epics::nt::NTScalar::createBuilder();
  Builder->addAlarm();
  auto PVStruct =
      Builder->value(epics::pvData::pvDouble)->addTimeStamp()->create();
  auto ValueField = PVStruct->getValue<epics::pvData::PVDouble>();
  ValueField->put(3.14);
  epics::pvData::TimeStamp TS;
  TS.getCurrent();
  epics::pvData::PVTimeStamp pvTS;
  PVStruct->attachTimeStamp(pvTS);

  // Construct an alarm structure
  auto AlarmData = epics::pvData::Alarm();
  AlarmData.setMessage(AlarmMessage);
  AlarmData.setStatus(epics::pvData::AlarmStatus::noStatus);
  AlarmData.setSeverity(epics::pvData::AlarmSeverity::majorAlarm);
  auto ConstructedPVAlarm = epics::pvData::PVAlarm();
  PVStruct->attachAlarm(ConstructedPVAlarm);
  ConstructedPVAlarm.set(AlarmData);

  return PVStruct->getPVStructure();
}

TEST(f142Test, some_test) {
  auto PVStruct = CreateTestPVStructWithAlarm("HIHI_ALARM");
  // auto AlarmStatusFromStruct = FlatBufs::f142::getAlarmStatus(PVStruct);
  
}
