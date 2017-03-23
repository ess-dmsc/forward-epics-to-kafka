#pragma once

#include "fbschemas.h"
#include "MakeFlatBufferFromPVStructure.h"
#include "SchemaRegistry.h"

namespace BrightnESS {
namespace ForwardEpicsToKafka {

class Converter {
public:
using ptr = std::unique_ptr<Converter>;
using sptr = std::shared_ptr<Converter>;
static ptr create(FlatBufs::SchemaRegistry const & schema_registry, std::string schema);
BrightnESS::FlatBufs::FB_uptr convert(FlatBufs::EpicsPVUpdate const & up);
private:
std::string schema;
FlatBufs::MakeFlatBufferFromPVStructure::ptr conv;
};

}
}
