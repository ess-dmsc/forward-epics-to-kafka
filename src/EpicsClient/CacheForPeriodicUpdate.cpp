#include "CacheForPeriodicUpdate.h"
#include <utility>
namespace BrightnESS {
namespace ForwardEpicsToKafka {
namespace EpicsClient {

///\fn emit
///\param up the epics PV update containing the PV structure
///\brief calls push_enlarge on the ring buffer to push a pv update object
int CacheForPeriodicUpdate::emit(
    std::shared_ptr<BrightnESS::FlatBufs::EpicsPVUpdate> up) {
  if (!up) {
    CLOG(6, 1, "empty update?");
    // should never happen, ignore
    return 0;
  }
  EpicsClient->emit(up);
  up.reset();
  return 1;
}

void CacheForPeriodicUpdate::setCachedUpdate(
    std::shared_ptr<FlatBufs::EpicsPVUpdate> PVUpdate) {
  CachedPVUpdate = PVUpdate;
}

///\fn PollPVCallback
///\brief checks for pv value, constructs the pv update object and emits it to
/// the ring buffer
void CacheForPeriodicUpdate::EmitCachedValue() { emit(CachedPVUpdate); }
}
}
}
