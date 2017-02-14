#pragma once

#include <memory>
#include <utility>
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


/// Interface for flat buffer creators for the different schemas
class MakeFlatBufferFromPVStructure {
public:
typedef std::unique_ptr<MakeFlatBufferFromPVStructure> ptr;
typedef std::shared_ptr<MakeFlatBufferFromPVStructure> sptr;
virtual BrightnESS::FlatBufs::FB_uptr convert(EpicsPVUpdate const & up) = 0;
};


class SchemaInfo {
public:
typedef std::unique_ptr<SchemaInfo> ptr;
virtual MakeFlatBufferFromPVStructure::ptr create_converter() = 0;
};

class SchemaRegistry {
public:
static std::map<std::string, SchemaInfo::ptr> & items();

static void registrate(std::string fbid, SchemaInfo::ptr && si) {
	items()[fbid] = std::move(si);
}

template <typename T>
class Registrar {
public:
Registrar(std::string fbid, SchemaInfo::ptr && si) {
	SchemaRegistry::registrate(fbid, std::move(si));
}
};

};


}
}
