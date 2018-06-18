#pragma once
#include "EpicsClientFactory.h"
#include "EpicsClientInterface.h"
#include "Stream.h"
#include <array>
#include <atomic>
#include <string>
#include <vector>

///\file Epics client monitor classes (PIMPL idiom avoids exposing pvAccess.h
/// and other details to other parts of the codebase)

namespace Forwarder {
namespace EpicsClient {

using std::array;
using std::vector;

class EpicsClientMonitor_impl;

/// Epics client implementation which monitors for PV updates.
class EpicsClientMonitor : public EpicsClientInterface {
public:
  /// Creates a new implementation and stores it as impl.
  /// This can then call the functions in the implementation.
  explicit EpicsClientMonitor(
      ChannelInfo &channelInfo,
      std::shared_ptr<
          moodycamel::ConcurrentQueue<std::unique_ptr<FlatBufs::EpicsPVUpdate>>>
          ring);
  ~EpicsClientMonitor() override;

  /// Pushes the PV update onto the emit_queue ring buffer.
  ///
  ///\param up An epics PV update holding the pv structure.
  int emit(std::unique_ptr<FlatBufs::EpicsPVUpdate> up) override;

  /// Calls stop on the client implementation.
  int stop() override;

  /// Setter method for status if there is an error in EPICS.
  void error_in_epics() override;

  /// Getter method for EPICS status_.
  int status() override { return status_; };

private:
  std::unique_ptr<EpicsClientMonitor_impl> impl;
  std::shared_ptr<
      moodycamel::ConcurrentQueue<std::unique_ptr<FlatBufs::EpicsPVUpdate>>>
      emit_queue;
  std::atomic<int> status_{0};
};
}
}
