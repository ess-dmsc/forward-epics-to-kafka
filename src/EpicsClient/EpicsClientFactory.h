#pragma once
#include <atomic>
#include <memory>
#include <mutex>

namespace BrightnESS {
namespace ForwardEpicsToKafka {
namespace EpicsClient {

struct EpicsClientFactoryInit {
  EpicsClientFactoryInit();
  ~EpicsClientFactoryInit();
  static std::unique_ptr<EpicsClientFactoryInit> factory_init();
  static std::atomic<int> count;
  static std::mutex mxl;
};
}
}
}