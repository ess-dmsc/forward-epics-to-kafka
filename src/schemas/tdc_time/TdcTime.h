
#pragma once

#include "../../FlatBufferCreator.h"
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
}
