#include "EpicsClientRandom.h"

namespace Forwarder {
namespace EpicsClient {

int EpicsClientRandom::emit(std::unique_ptr<FlatBufs::EpicsPVUpdate> up) {
  emit_queue->push_enlarge(up);
  return 1;
}
}
}
