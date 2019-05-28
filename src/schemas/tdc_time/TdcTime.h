
#pragma once

#include "../../FlatBufferCreator.h"
#include "../../SchemaRegistry.h"
#include "../../helper.h"
#include <map>
#include <string>

namespace TdcTime {
class Converter : public FlatBufs::FlatBufferCreator {
public:
  Converter() = default;
  ~Converter() = default;
  std::unique_ptr<FlatBufs::FlatbufferMessage>
  create(FlatBufs::EpicsPVUpdate const &up) override;
  void
  config(std::map<std::string, std::string> const &KafkaConfiguration) override;
};

std::unique_ptr<FlatBufs::FlatbufferMessage>
generateFlatbufferFromData(std::string const &Name,
                           std::vector<std::uint64_t> Timestamps);

class Info : public FlatBufs::SchemaInfo {
public:
  std::unique_ptr<FlatBufs::FlatBufferCreator> createConverter() override;
};
}
