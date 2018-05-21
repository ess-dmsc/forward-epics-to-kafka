#include "EpicsClientFactory.h"
#include "logger.h"
// For epics::pvAccess::ClientFactory::start()
#include <pv/caProvider.h>
#include <pv/clientFactory.h>

namespace BrightnESS {
namespace ForwardEpicsToKafka {
namespace EpicsClient {

using std::mutex;
using ulock = std::unique_lock<mutex>;

std::atomic<int> EpicsClientFactoryInit::count{0};

std::mutex EpicsClientFactoryInit::mxl;

std::unique_ptr<EpicsClientFactoryInit> EpicsClientFactoryInit::factory_init() {
  return std::unique_ptr<EpicsClientFactoryInit>(new EpicsClientFactoryInit);
}

EpicsClientFactoryInit::EpicsClientFactoryInit() {
  CLOG(7, 7, "EpicsClientFactoryInit");
  ulock lock(mxl);
  auto c = count++;
  if (c == 0) {
    CLOG(6, 6, "START  Epics factories");
    ::epics::pvAccess::ClientFactory::start();
    ::epics::pvAccess::ca::CAClientFactory::start();
  }
}

EpicsClientFactoryInit::~EpicsClientFactoryInit() {
  CLOG(7, 7, "~EpicsClientFactoryInit");
  ulock lock(mxl);
  auto c = --count;
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
}
}
}