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
  /// Do not rely on channel, will likely go away...
  std::string channel;
  /// Timestamp when monitorEvent() was called
  uint64_t ts_epics_monitor = 0;
};
}
