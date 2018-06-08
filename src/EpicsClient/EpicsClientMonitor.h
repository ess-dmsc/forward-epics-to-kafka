#pragma once
#include "EpicsClientFactory.h"
#include "EpicsClientInterface.h"
#include "Stream.h"
#include <array>
#include <atomic>
#include <pv/pvAccess.h>
#include <string>
#include <vector>

///\file Epics client monitor classes (PIMPL idiom)

namespace Forwarder {
namespace EpicsClient {

using std::array;
using std::vector;

class EpicsClientMonitor;

/// Implementation for EPICS client monitor.
class EpicsClientMonitor_impl {
public:
  explicit EpicsClientMonitor_impl(EpicsClientInterface *epics_client)
      : epics_client(epics_client) {}
  ~EpicsClientMonitor_impl();

  /// Starts the EPICS channel access provider loop and the monitor requester
  /// loop for monitoring EPICS PVs.
  int init(std::string epics_channel_provider_type);

  /// Creates a new monitor requester instance and starts the epics monitoring
  /// loop.
  int monitoring_start();

  /// Stops the EPICS monitor loop in monitor_requester and resets the pointer.
  int monitoring_stop();

  /// Logs that the channel has been destroyed and stops monitoring.
  int channel_destroyed();

  /// Stops the EPICS monitor loop.
  int stop();

  /// Pushes update to the emit_queue ring buffer which is owned by a stream.
  int emit(std::unique_ptr<FlatBufs::EpicsPVUpdate>);

  /// Logging function.
  void error_channel_requester();
  epics::pvData::MonitorRequester::shared_pointer monitor_requester;
  epics::pvAccess::ChannelProvider::shared_pointer provider;
  epics::pvAccess::ChannelRequester::shared_pointer channel_requester;
  epics::pvAccess::Channel::shared_pointer channel;
  epics::pvData::Monitor::shared_pointer monitor;
  std::recursive_mutex mx;
  std::string channel_name;
  EpicsClientInterface *epics_client = nullptr;
  std::unique_ptr<EpicsClientFactoryInit> factory_init;
};

/// Epics client implementation which monitors for PV updates.
class EpicsClientMonitor : public EpicsClientInterface {
public:
  /// Creates a new implementation and stores it as impl.
  /// This can then call the functions in the implementation.
  explicit EpicsClientMonitor(
      ChannelInfo &channelInfo,
      std::shared_ptr<Ring<std::unique_ptr<FlatBufs::EpicsPVUpdate>>> ring);
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
  std::shared_ptr<Ring<std::unique_ptr<FlatBufs::EpicsPVUpdate>>> emit_queue;
  std::atomic<int> status_{0};
};
}
}
