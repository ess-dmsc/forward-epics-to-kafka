#pragma once

#include "fbschemas.h"
#include "MakeFlatBufferFromPVStructure.h"
#include "SchemaRegistry.h"
#include "MainOpt.h"

namespace BrightnESS {
namespace ForwardEpicsToKafka {

class Converter {
public:
  using ptr = std::unique_ptr<Converter>;
  using sptr = std::shared_ptr<Converter>;
  static sptr create(FlatBufs::SchemaRegistry const &schema_registry,
                     std::string schema, MainOpt const &main_opt);
  BrightnESS::FlatBufs::FB_uptr convert(FlatBufs::EpicsPVUpdate const &up);

private:
  std::string schema;
  FlatBufs::MakeFlatBufferFromPVStructure::ptr conv;
};
}
}
