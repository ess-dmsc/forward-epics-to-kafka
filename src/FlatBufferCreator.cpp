// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

#include "FlatBufferCreator.h"
#include "logger.h"

namespace FlatBufs {

void FlatBufferCreator::config(
    std::map<std::string, std::string> const &KafkaConfiguration) {
  UNUSED_ARG(KafkaConfiguration);
}

std::map<std::string, double> FlatBufferCreator::getStats() { return {}; }
} // namespace FlatBufs
