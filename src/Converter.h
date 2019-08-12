// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

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
