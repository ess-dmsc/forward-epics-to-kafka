#pragma once
#include <atomic>
#include "../logger.h"
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
