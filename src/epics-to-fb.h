#pragma once

#include <memory>
#include <string>
#include "fbschemas.h"
#include <pv/pvData.h>

namespace BrightnESS {
namespace FlatBufs {

struct EpicsPVstr;
class ConversionPath;

/// Represents and Epics update with the new PV value
struct EpicsPVUpdate {
EpicsPVUpdate();
EpicsPVUpdate(EpicsPVUpdate const &);
EpicsPVUpdate(EpicsPVUpdate &&) = delete;
~EpicsPVUpdate();
::epics::pvData::PVStructure::shared_pointer epics_pvstr;
/// Do not rely on channel, will likely go away...
std::string channel;
uint64_t seq;
/// Timestamp when monitorEvent() was called
uint64_t ts_epics_monitor;
uint32_t fwdix;
uint64_t teamid = 0;
//void * monitor = nullptr;
//void * ele = nullptr;
};


}
}
