#pragma once

#include "FlatbufferMessage.h"
#include "MainOpt.h"
#include "MakeFlatBufferFromPVStructure.h"
#include "SchemaRegistry.h"
#include <map>
#include <string>

namespace Forwarder {

class Converter {
public:
  using ptr = std::unique_ptr<Converter>;
  using sptr = std::shared_ptr<Converter>;
  static sptr create(FlatBufs::SchemaRegistry const &schema_registry,
                     std::string schema, MainOpt const &main_opt);
  FlatBufs::FlatbufferMessage::uptr convert(FlatBufs::EpicsPVUpdate const &up);
  std::map<std::string, double> stats();
  std::string schema_name() const;

private:
  std::string schema;
  FlatBufs::MakeFlatBufferFromPVStructure::ptr conv;
};
} // namespace Forwarder
