#include "DataFromPVStruct.h"

namespace FlatBufs {
namespace f142 {

AlarmStatus
getAlarmStatus(epics::pvData::PVStructurePtr const &PVStructureField) {
  auto AlarmField = PVStructureField->getSubField("alarm");
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
}
}
