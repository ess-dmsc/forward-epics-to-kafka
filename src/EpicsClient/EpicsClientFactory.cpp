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
  
EpicsClientFactoryInit::EpicsClientFactoryInit() {
  Logger->debug("START  Epics factories");
  ::epics::pvAccess::ca::CAClientFactory::start();
}
} // namespace EpicsClient
} // namespace Forwarder
