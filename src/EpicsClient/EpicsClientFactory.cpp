#include "EpicsClientFactory.h"
#include "helper.h"
#include "logger.h"
// For epics::pvAccess::ClientFactory::start()
#include <pv/caProvider.h>
#include <pv/clientFactory.h>

namespace Forwarder {
namespace EpicsClient {

using std::mutex;
using ulock = std::unique_lock<mutex>;

std::atomic<int> EpicsClientFactoryInit::Count{0};

std::mutex EpicsClientFactoryInit::MutexLock;

std::unique_ptr<EpicsClientFactoryInit> EpicsClientFactoryInit::factory_init() {
  return ::make_unique<EpicsClientFactoryInit>();
}

EpicsClientFactoryInit::EpicsClientFactoryInit() {
  CLOG(7, 7, "EpicsClientFactoryInit");
  ulock lock(MutexLock);
  auto c = Count++;
  if (c == 0) {
    CLOG(6, 6, "START  Epics factories");
    ::epics::pvAccess::ClientFactory::start();
    ::epics::pvAccess::ca::CAClientFactory::start();
  }
}

EpicsClientFactoryInit::~EpicsClientFactoryInit() {
  CLOG(7, 7, "~EpicsClientFactoryInit");
  ulock lock(MutexLock);
  auto c = --Count;
  if (c < 0) {
    LOG(0, "Reference count {} is not consistent, should never happen, but "
           "ignoring for now.",
        c);
    c = 0;
  }
  if (c == 0) {
    CLOG(7, 6, "STOP   Epics factories");
    ::epics::pvAccess::ClientFactory::stop();
    ::epics::pvAccess::ca::CAClientFactory::stop();
  }
}
} // namespace EpicsClient
} // namespace Forwarder
