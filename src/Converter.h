#pragma once

#include "FlatbufferMessage.h"
#include "MainOpt.h"
#include "MakeFlatBufferFromPVStructure.h"
#include "SchemaRegistry.h"
#include <nlohmann/json.hpp>
#include <map>
#include <string>

namespace BrightnESS {
namespace ForwardEpicsToKafka {

class Converter {
public:
  using ptr = std::unique_ptr<Converter>;
  using sptr = std::shared_ptr<Converter>;
  static sptr create(FlatBufs::SchemaRegistry const &schema_registry,
                     std::string schema, MainOpt const &main_opt);
  FlatBufs::FlatbufferMessage::uptr
  convert(FlatBufs::EpicsPVUpdate const &up);
  std::map<std::string, double> stats();
  std::string schema_name();
  static void extractConfig(std::string &schema,
                            nlohmann::json const &config,
                            std::map<std::string, int64_t> &config_ints,
                            std::map<std::string, std::string> &config_strings);

private:
  std::string schema;
  FlatBufs::MakeFlatBufferFromPVStructure::ptr conv;
};
}
}
