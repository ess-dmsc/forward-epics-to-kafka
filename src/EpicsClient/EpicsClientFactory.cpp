// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

#include "EpicsClientFactory.h"
#include "../logger.h"
// For epics::pvAccess::ClientFactory::start()
#include <pv/caProvider.h>
#include <pv/clientFactory.h>

namespace Forwarder {
namespace EpicsClient {

std::atomic<int> EpicsClientFactoryInit::Count{0};

std::mutex EpicsClientFactoryInit::MutexLock;

std::unique_ptr<EpicsClientFactoryInit> EpicsClientFactoryInit::factory_init() {
  return std::make_unique<EpicsClientFactoryInit>();
}

EpicsClientFactoryInit::EpicsClientFactoryInit() {
  Logger->trace("EpicsClientFactoryInit");
  std::lock_guard<std::mutex> lock(MutexLock);
  auto c = Count++;
  if (c == 0) {
    Logger->debug("START  Epics factories");
    ::epics::pvAccess::ClientFactory::start();
    ::epics::pvAccess::ca::CAClientFactory::start();
  }
}

EpicsClientFactoryInit::~EpicsClientFactoryInit() {
  Logger->trace("~EpicsClientFactoryInit");
  std::lock_guard<std::mutex> lock(MutexLock);
  auto c = --Count;
  if (c < 0) {
    Logger->error(
        "Reference count {} is not consistent, should never happen, but "
        "ignoring for now.",
        c);
    c = 0;
  }
  if (c == 0) {
    Logger->debug("STOP   Epics factories");
    ::epics::pvAccess::ClientFactory::stop();
    ::epics::pvAccess::ca::CAClientFactory::stop();
  }
}
} // namespace EpicsClient
} // namespace Forwarder
