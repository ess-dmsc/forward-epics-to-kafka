#include "DataFromPVStruct.h"
#include <pv/nt.h>

namespace FlatBufs {
namespace f142 {

std::map<epics::pvData::AlarmSeverity, AlarmSeverity>
    EpicsSeverityToFlatbufferSeverity = {
        {epics::pvData::AlarmSeverity::minorAlarm, AlarmSeverity::MINOR},
        {epics::pvData::AlarmSeverity::majorAlarm, AlarmSeverity::MAJOR},
        {epics::pvData::AlarmSeverity::noAlarm, AlarmSeverity::NO_ALARM},
        {epics::pvData::AlarmSeverity::invalidAlarm, AlarmSeverity::INVALID},
        {epics::pvData::AlarmSeverity::undefinedAlarm, AlarmSeverity::INVALID}};

AlarmStatus
getAlarmStatus(epics::pvData::PVStructurePtr const &PVStructureField) {
  auto AlarmField = PVStructureField->getSubField("alarm");
  if (AlarmField == nullptr) {
    return AlarmStatus::NO_ALARM;
  }
  auto MessageField =
      (dynamic_cast<epics::pvData::PVStructure *>(AlarmField.get()))
          ->getSubField("message");
  auto AlarmString = dynamic_cast<epics::pvData::PVScalarValue<std::string> *>(
                         MessageField.get())
                         ->get();
  // Message field is HIHI_ALARM, LOW_ALARM, etc. We have to drop _ALARM in
  // every case apart from NO_ALARM
  if (AlarmString != "NO_ALARM")
    AlarmString = AlarmString.substr(0, AlarmString.length() - 6);

  // Match the alarm string from EPICS with an enum value in our flatbuffer
  // schema
  auto StatusNames = EnumNamesAlarmStatus();
  int i = 0;
  while (StatusNames[i] != nullptr) {
    if (AlarmString == StatusNames[i]) {
      return static_cast<AlarmStatus>(i);
    }
    i++;
  }
  return AlarmStatus::UDF;
}

AlarmSeverity
getAlarmSeverity(epics::pvData::PVStructurePtr const &PVStructureField) {
  auto AlarmField = PVStructureField->getSubField("alarm");
  if (AlarmField == nullptr) {
    return AlarmSeverity::NO_ALARM;
  }
  auto SeverityField =
      (dynamic_cast<epics::pvData::PVStructure *>(AlarmField.get()))
          ->getSubField("severity");
  auto SeverityValue =
      dynamic_cast<epics::pvData::PVScalarValue<int32_t> *>(SeverityField.get())
          ->get();
  auto Severity = epics::pvData::AlarmSeverity(SeverityValue);

  return EpicsSeverityToFlatbufferSeverity[Severity];
}
} // namespace f142
} // namespace FlatBufs
