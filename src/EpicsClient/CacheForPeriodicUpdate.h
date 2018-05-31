#pragma once
#include "EpicsClientInterface.h"
#include "EpicsClientMonitor.h"
#include "Ring.h"
#include <memory>

namespace BrightnESS {
namespace ForwardEpicsToKafka {
namespace EpicsClient {

class CacheForPeriodicUpdate {
public:
  explicit CacheForPeriodicUpdate(EpicsClientInterface *EpicsClient)
      : EpicsClient(EpicsClient){};
  int emit(std::shared_ptr<FlatBufs::EpicsPVUpdate> up);
  void setCachedUpdate(std::shared_ptr<FlatBufs::EpicsPVUpdate> PVUpdate);

private:
  void EmitCachedValue();
  EpicsClientInterface *EpicsClient;
  std::shared_ptr<FlatBufs::EpicsPVUpdate> CachedPVUpdate;
};
}
}
}