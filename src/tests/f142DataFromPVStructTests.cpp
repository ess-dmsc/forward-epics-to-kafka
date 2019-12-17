#include "schemas/f142/DataFromPVStruct.h"
#include <gtest/gtest.h>
#include <pv/nt.h>
#include <pv/pvAlarm.h>
#include <pv/pvTimeStamp.h>

epics::pvData::PVStructure::shared_pointer
CreateTestPVStructWithAlarm(std::string const &AlarmMessage,
                            epics::pvData::AlarmSeverity AlarmSeverityLevel =
                                epics::pvData::AlarmSeverity::noAlarm) {
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
  AlarmData.setSeverity(AlarmSeverityLevel);
  auto ConstructedPVAlarm = epics::pvData::PVAlarm();
  PVStruct->attachAlarm(ConstructedPVAlarm);
  ConstructedPVAlarm.set(AlarmData);

  return PVStruct->getPVStructure();
}

void testAlarmStatusFromAlarmMessage(std::string const &AlarmMessage,
                                     AlarmStatus ExpectedAlarmStatus) {
  auto PVStruct = CreateTestPVStructWithAlarm(AlarmMessage);
  auto AlarmStatusFromStruct = FlatBufs::f142::getAlarmStatus(PVStruct);
  ASSERT_EQ(AlarmStatusFromStruct, ExpectedAlarmStatus);
}

TEST(
    f142Test,
    test_getAlarmStatus_returns_expected_status_based_on_alarm_message_in_pv_structure) {
  testAlarmStatusFromAlarmMessage("NO_ALARM", AlarmStatus::NO_ALARM);
  testAlarmStatusFromAlarmMessage("HIHI_ALARM", AlarmStatus::HIHI);
  testAlarmStatusFromAlarmMessage("LOW_ALARM", AlarmStatus::LOW);
  testAlarmStatusFromAlarmMessage("READ_ACCESS_ALARM",
                                  AlarmStatus::READ_ACCESS);
}

void testAlarmSeverityFromAlarmStruct(
    epics::pvData::AlarmSeverity AlarmSeverityInStruct,
    AlarmSeverity ExpectedAlarmSeverity) {
  auto PVStruct =
      CreateTestPVStructWithAlarm("NO_ALARM", AlarmSeverityInStruct);
  auto AlarmStatusFromStruct = FlatBufs::f142::getAlarmSeverity(PVStruct);
  ASSERT_EQ(AlarmStatusFromStruct, ExpectedAlarmSeverity);
}

TEST(
    f142Test,
    test_getAlarmSeverity_returns_expected_severity_based_on_alarm_in_pv_structure) {
  testAlarmSeverityFromAlarmStruct(epics::pvData::AlarmSeverity::noAlarm,
                                   AlarmSeverity::NO_ALARM);
  testAlarmSeverityFromAlarmStruct(epics::pvData::AlarmSeverity::minorAlarm,
                                   AlarmSeverity::MINOR);
  testAlarmSeverityFromAlarmStruct(epics::pvData::AlarmSeverity::majorAlarm,
                                   AlarmSeverity::MAJOR);
  testAlarmSeverityFromAlarmStruct(epics::pvData::AlarmSeverity::invalidAlarm,
                                   AlarmSeverity::INVALID);
  testAlarmSeverityFromAlarmStruct(epics::pvData::AlarmSeverity::undefinedAlarm,
                                   AlarmSeverity::INVALID);
}
