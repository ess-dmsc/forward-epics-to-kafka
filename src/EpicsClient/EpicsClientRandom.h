#pragma once

#include "EpicsClientInterface.h"
#include "Ring.h"

namespace FlatBufs {
class EpicsPVUpdate;
}

namespace Forwarder {
namespace EpicsClient {

/// A fake EpicsClient implementation which generates PVUpdates containing
/// random numbers, for testing purposes
class EpicsClientRandom : public EpicsClientInterface {
public:
  explicit EpicsClientRandom(
      std::shared_ptr<Ring<std::unique_ptr<FlatBufs::EpicsPVUpdate>>>
          RingBuffer)
      : emit_queue(RingBuffer){};
  ~EpicsClientRandom() override = default;
  int emit(std::unique_ptr<FlatBufs::EpicsPVUpdate> up) override;
  int stop() override { return 0; };
  void error_in_epics() override { status_ = -1; };
  int status() override { return status_; };

private:
  /// Buffer of (fake) PVUpdates
  std::shared_ptr<Ring<std::unique_ptr<FlatBufs::EpicsPVUpdate>>> emit_queue;
  // int status_ = 0;
  int status_{0};
};
}
}
