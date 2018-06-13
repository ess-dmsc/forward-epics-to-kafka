#pragma once

#include <memory>
#include <pv/pvData.h>
#include <string>

namespace FlatBufs {

class ConversionPath;

/// Represents and Epics update with the new PV value
struct EpicsPVUpdate {
  EpicsPVUpdate() = default;
  EpicsPVUpdate(EpicsPVUpdate const &x) = default;
  EpicsPVUpdate(EpicsPVUpdate &&) = delete;
  ~EpicsPVUpdate() = default;
  ::epics::pvData::PVStructure::shared_pointer epics_pvstr;
  /// Do not rely on channel, will likely go away...
  std::string channel;
  uint64_t seq_data = 0;
  uint64_t seq_fwd = 0;
  /// Timestamp when monitorEvent() was called
  uint64_t ts_epics_monitor = 0;
};
}
