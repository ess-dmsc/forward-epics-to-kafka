// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

#pragma once

#include "../Stream.h"
#include "EpicsClientInterface.h"
#include <concurrentqueue/concurrentqueue.h>
#include <random>

namespace Forwarder {
namespace EpicsClient {

/// A fake EpicsClient implementation which generates PVUpdates containing
/// random numbers, for testing purposes
class EpicsClientRandom : public EpicsClientInterface {
public:
  explicit EpicsClientRandom(
      ChannelInfo &channelInfo,
      std::shared_ptr<
          moodycamel::ConcurrentQueue<std::shared_ptr<FlatBufs::EpicsPVUpdate>>>
          RingBuffer)
      : ChannelInformation(channelInfo), EmitQueue(std::move(RingBuffer)),
        UniformDistribution(0, 100){};
  ~EpicsClientRandom() override = default;
  void emit(std::shared_ptr<FlatBufs::EpicsPVUpdate> Update) override;
  int stop() override { return 0; };
  void errorInEpics() override { status_ = -1; };
  int status() override { return status_; };

  /// Generate a fake EpicsPVUpdate and emit it
  void generateFakePVUpdate();

  std::string getConnectionState() override { return "EpicsClientRandom"; }

private:
  /// Get current time since unix epoch in nanoseconds
  uint64_t getCurrentTimestamp() const;
  /// Create a PVStructure with the specified value
  epics::pvData::PVStructurePtr createFakePVStructure(double Value) const;

  ChannelInfo ChannelInformation;
  /// Buffer of (fake) PVUpdates
  std::shared_ptr<
      moodycamel::ConcurrentQueue<std::shared_ptr<FlatBufs::EpicsPVUpdate>>>
      EmitQueue;
  /// Status is set to 1 if something fails
  int status_{0};
  /// Tools for generating random doubles
  std::uniform_real_distribution<double> UniformDistribution;
  std::default_random_engine RandomEngine;
};
}
}
