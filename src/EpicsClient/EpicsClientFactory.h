#pragma once
#include <atomic>
#include <memory>
#include <mutex>

namespace Forwarder {
namespace EpicsClient {

/// Handles the channel access network provider.
/// Starts and stops provider on construction and destruction respectively.
struct EpicsClientFactoryInit {
  EpicsClientFactoryInit();
  ~EpicsClientFactoryInit();

  /// Returns a new instance of the EPICS client factory in a unique pointer.
  static std::unique_ptr<EpicsClientFactoryInit> factory_init();
  static std::atomic<int> count;
  static std::mutex mxl;
};
}
}
