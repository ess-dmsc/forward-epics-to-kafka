#pragma once

#include "EpicsClientInterface.h"
#include "Ring.h"
#include <Stream.h>
#include <random>

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
      ChannelInfo &channelInfo,
      std::shared_ptr<Ring<std::unique_ptr<FlatBufs::EpicsPVUpdate>>>
          RingBuffer)
      : ChannelInformation(channelInfo), emit_queue(RingBuffer),
        UniformDistribution(0, 100){};
  ~EpicsClientRandom() override = default;
  int emit(std::unique_ptr<FlatBufs::EpicsPVUpdate> up) override;
  int stop() override { return 0; };
  void error_in_epics() override { status_ = -1; };
  int status() override { return status_; };

  /// Generate a fake EpicsPVUpdate and emit it
  void generateFakePVUpdate();

private:
  /// Get current time since unix epoch in nanoseconds
  uint64_t getCurrentTimestamp() const;
  /// Create a PVStructure with the specified value
  epics::pvData::PVStructurePtr createFakePVStructure(double Value) const;

  ChannelInfo ChannelInformation;
  /// Buffer of (fake) PVUpdates
  std::shared_ptr<Ring<std::unique_ptr<FlatBufs::EpicsPVUpdate>>> emit_queue;
  /// Status is set to 1 if something fails
  int status_{0};
  /// Tools for generating random doubles
  std::uniform_real_distribution<double> UniformDistribution;
  std::default_random_engine RandomEngine;
};
}
}
