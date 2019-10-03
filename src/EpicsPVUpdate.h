// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

#pragma once

#include <memory>
#include <pv/pvData.h>
#include <string>

namespace FlatBufs {

/// Represents and Epics update with the new PV value
struct EpicsPVUpdate {
  EpicsPVUpdate() = default;
  EpicsPVUpdate(EpicsPVUpdate const &x) = default;
  EpicsPVUpdate(EpicsPVUpdate &&) = delete;
  ~EpicsPVUpdate() = default;
  ::epics::pvData::PVStructure::shared_pointer epics_pvstr;
  std::string channel;
  /// True if the alarm message changed from the previous cached update
  bool AlarmStatusChanged = false;
};
}
