#pragma once

#include <memory>
#include <string>
#include "fbschemas.h"
#include <pv/pvData.h>

namespace BrightnESS {
namespace FlatBufs {


/// Represents and Epics update with the new PV value and channel name
class EpicsPVUpdate {
public:
std::string channel;
epics::pvData::PVStructure::shared_pointer pvstr;
uint64_t seq;
/// Timestamp when monitorEvent() was called
uint64_t ts_epics_monitor;
uint32_t fwdix;
uint64_t teamid = 0;
};


}
}
