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

class EpicsClientMonitorImpl;

/// Epics client implementation which monitors for PV updates.
class EpicsClientMonitor : public EpicsClientInterface {
public:
  /// Creates a new implementation and stores it as impl.
  ///
  /// This can then call the functions in the implementation.
  explicit EpicsClientMonitor(
      ChannelInfo &ChannelInfo,
      std::shared_ptr<
          moodycamel::ConcurrentQueue<std::shared_ptr<FlatBufs::EpicsPVUpdate>>>
          EmitQueue);
  ~EpicsClientMonitor() override;

  /// Pushes the PV update onto the emit_queue ring buffer.
  ///
  /// \param Update An epics PV update holding the pv structure.
  void emit(std::shared_ptr<FlatBufs::EpicsPVUpdate> Update) override;

  void emitWithoutCaching(std::shared_ptr<FlatBufs::EpicsPVUpdate> Update);

  /// Calls stop on the client implementation.
  int stop() override;

  /// Setter method for status if there is an error in EPICS.
  void errorInEpics() override;

  /// Getter method for EPICS status.
  int status() override { return status_; };

  void emitCachedValue();

  std::string getConnectionState() override;

  void handleChannelRequesterError(std::string const &) override;
  void handleConnectionStateChange(
      ChannelConnectionState NewConnectionState) override;

  std::unique_ptr<KafkaW::ProducerTopic> ConnectionStatusProducer;

  void setServiceID(std::string NewServiceID) override;

private:
  std::unique_ptr<EpicsClientMonitorImpl> Impl;
  std::shared_ptr<
      moodycamel::ConcurrentQueue<std::shared_ptr<FlatBufs::EpicsPVUpdate>>>
      EmitQueue;
  std::shared_ptr<FlatBufs::EpicsPVUpdate> CachedUpdate;
  std::mutex CachedUpdateMutex;
  std::atomic<int> status_{0};
  ChannelConnectionState ConnectionState =
      ChannelConnectionState::NEVER_CONNECTED;
  std::string ServiceID;
  SharedLogger Logger = getLogger();
};
} // namespace EpicsClient
} // namespace Forwarder
