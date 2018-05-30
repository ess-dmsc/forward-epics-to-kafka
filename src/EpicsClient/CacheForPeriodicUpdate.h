#pragma once
#include "EpicsClientInterface.h"
#include "Ring.h"
#include <memory>

namespace BrightnESS {
namespace ForwardEpicsToKafka {
namespace EpicsClient {

class CacheForPeriodicUpdate {
public:
  explicit CacheForPeriodicUpdate(
      std::shared_ptr<Ring<std::shared_ptr<FlatBufs::EpicsPVUpdate>>> Ring)
      : EmitQueue(Ring){};
  int emit(std::shared_ptr<FlatBufs::EpicsPVUpdate> up);
  void setCachedUpdate(std::shared_ptr<FlatBufs::EpicsPVUpdate> PVUpdate);

private:
  void EmitCachedValue();
  std::shared_ptr<Ring<std::shared_ptr<FlatBufs::EpicsPVUpdate>>> EmitQueue;
  std::shared_ptr<FlatBufs::EpicsPVUpdate> CachedPVUpdate;
};
}
}
}