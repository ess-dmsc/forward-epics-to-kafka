#pragma once

#include "FlatbufferMessage.h"
#include "MainOpt.h"
#include "FlatBufferCreator.h"
#include "SchemaRegistry.h"
#include <map>
#include <string>

namespace Forwarder {

class Converter {
public:
  static std::shared_ptr<Converter>
  create(FlatBufs::SchemaRegistry const &schema_registry, std::string schema,
         MainOpt const &main_opt);
  std::unique_ptr<FlatBufs::FlatbufferMessage>
  convert(FlatBufs::EpicsPVUpdate const &up);
  std::map<std::string, double> stats();
  std::string schema_name() const;

private:
  std::string schema;
  std::unique_ptr<FlatBufs::FlatBufferCreator> conv;
};
} // namespace Forwarder
