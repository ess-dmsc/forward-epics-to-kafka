#pragma once

#include "FlatBufferCreator.h"
#include "FlatbufferMessage.h"
#include "MainOpt.h"
#include "SchemaRegistry.h"
#include <map>
#include <string>

namespace Forwarder {

class Converter {
public:
  static std::shared_ptr<Converter>
  create(FlatBufs::SchemaRegistry const &schema_registry, std::string Schema,
         MainOpt const &Options);
  std::unique_ptr<FlatBufs::FlatbufferMessage>
  convert(FlatBufs::EpicsPVUpdate const &Update);
  std::map<std::string, double> stats();
  std::string getSchemaID() const;

private:
  std::string SchemaID;
  std::unique_ptr<FlatBufs::FlatBufferCreator> FlatBufCreator;
};
} // namespace Forwarder
