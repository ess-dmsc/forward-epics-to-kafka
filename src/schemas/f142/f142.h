// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

#pragma once

#include "../../FlatBufferCreator.h"
#include "../../RangeSet.h"
#include "../../SchemaRegistry.h"
#include <map>

namespace FlatBufs {
namespace f142 {

struct Statistics {
  uint64_t err_timestamp_not_available = 0;
  uint64_t err_not_implemented_yet = 0;
};

class Converter : public FlatBufferCreator {
public:
  Converter() = default;
  ~Converter() override { Logger->error("~Converter"); }

  std::unique_ptr<FlatBufs::FlatbufferMessage>
  create(EpicsPVUpdate const &PVUpdate) override;

  std::map<std::string, double> getStats() override {
    return {{"ranges_n", seqs.size()}};
  }

  RangeSet<uint64_t> seqs;
  Statistics Stats;

private:
  SharedLogger Logger = getLogger();
};
} // namespace f142
} // namespace FlatBufs
