// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

#pragma once
#include "../logger.h"
#include <atomic>
#include <memory>
#include <mutex>

namespace Forwarder {
namespace EpicsClient {
class EpicsClientFactoryInit {
public:
  EpicsClientFactoryInit();

private:
  static bool HasBeenStarted;
  SharedLogger Logger = getLogger();
};
} // namespace EpicsClient
} // namespace Forwarder
