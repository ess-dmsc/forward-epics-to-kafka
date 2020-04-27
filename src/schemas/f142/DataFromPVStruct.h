// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

#pragma once

#include <f142_logdata_generated.h>
#include <pv/pvData.h>

namespace FlatBufs {
namespace f142 {

AlarmStatus
getAlarmStatus(epics::pvData::PVStructurePtr const &PVStructureField);

AlarmSeverity
getAlarmSeverity(epics::pvData::PVStructurePtr const &PVStructureField);
} // namespace f142
} // namespace FlatBufs
