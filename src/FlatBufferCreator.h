// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

#pragma once

#include "FlatbufferMessage.h"
#include <map>
#include <memory>
#include <string>

namespace FlatBufs {

struct EpicsPVUpdate;

/// Interface for flat buffer creators for the different schemas
class FlatBufferCreator {
public:
  virtual ~FlatBufferCreator() = default;
  virtual std::unique_ptr<FlatbufferMessage>
  create(EpicsPVUpdate const &up) = 0;
  virtual void
  config(std::map<std::string, std::string> const &KafkaConfiguration);
  virtual std::map<std::string, double> getStats();
};
} // namespace FlatBufs
