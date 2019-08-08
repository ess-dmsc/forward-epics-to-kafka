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

/// Handles the channel access network provider.
/// Starts and stops provider on construction and destruction respectively.
class EpicsClientFactoryInit {
public:
  EpicsClientFactoryInit();
  ~EpicsClientFactoryInit();

  /// Returns a new instance of the EPICS client factory.
  static std::unique_ptr<EpicsClientFactoryInit> factory_init();
  static std::atomic<int> Count;
  static std::mutex MutexLock;

private:
  SharedLogger Logger = getLogger();
};
}
}
