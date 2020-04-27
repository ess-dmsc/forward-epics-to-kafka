// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

#pragma once
#include "../logger.h"
#include "ChannelRequester.h"
#include "FwdMonitorRequester.h"
#include <pv/pvAccess.h>
#include <pva/client.h>
#include <pv/configuration.h>
#include <pv/pvData.h>
#include <pv/reftrack.h>

namespace Forwarder {
namespace EpicsClient {
  namespace pva = epics::pvAccess;
  namespace pvd = epics::pvData;

using urlock = std::unique_lock<std::recursive_mutex>;
#define RLOCK() urlock lock(mx);

/// Implementation for EPICS client monitor.
class EpicsClientMonitorImpl {
public:
  explicit EpicsClientMonitorImpl(EpicsClientInterface *EpicsClient, std::string ProviderType, std::string ChannelName)
  : EpicsClient(EpicsClient), Channel(EpicsClientMonitorImpl::getClientProvider(ProviderType)->connect(ChannelName)), channel_name(ChannelName)  {
      }
  ~EpicsClientMonitorImpl() {
    monitoringStop();
    Channel.reset();
    Logger->trace("EpicsClientMonitor_implor_impl"); }

  /// Creates a new monitor requester instance and starts the epics monitoring
  /// loop.
  int monitoringStart() {
    RLOCK();
    if (!Channel.valid()) {
      Logger->debug("monitoringStart:  want to start but we have no channel");
      return -1;
    }
    Logger->trace("monitoringStart");
    // Leaving it empty seems to be the full channel, including name.  That's
    // good.
    // Can also specify subfields, e.g. "value, timeStamp"  or also
    // "field(value)"
    // We need to be more explicit here for compatibility with channel access.
//    std::string RequestStr = "field(value,timeStamp,alarm)";
    std::string RequestStr = "";
    epics::pvData::PVStructure::shared_pointer pvReq(pvd::createRequest(RequestStr));
    if (monitor) {
      monitoringStop();
    }
    monitor_requester.reset(new FwdMonitorRequester(EpicsClient, channel_name));
    monitor = channel->createMonitor(monitor_requester, pvReq);
    if (!monitor) {
      Logger->warn("could not create EPICS monitor instance");
      return -2;
    }
    return 0;
  }

  /// Stops the EPICS monitor loop in monitor_requester and resets the pointer.
  int monitoringStop() {
    RLOCK();
    Logger->trace("monitoringStop");
    if (monitor) {
      monitor->stop();
      monitor->destroy();
    }
    monitor_requester.reset();
    monitor.reset();
    return 0;
  }

  /// Logs that the channel has been destroyed and stops monitoring.
  int channelDestroyed() {
    Logger->warn("channelDestroyed()");
    monitoringStop();
    return 0;
  }

  /// Stops the EPICS monitor loop.
  int stop() {
    RLOCK();
    if (monitor) {
      monitor->stop();
      monitor->destroy();
    }
    if (channel) {
      channel->destroy();
    }
    monitor.reset();
    channel.reset();
    return 0;
  }

  /// Pushes update to the emit_queue ring buffer which is owned by a stream.
  void emit(std::shared_ptr<FlatBufs::EpicsPVUpdate> const &Update) {
    EpicsClient->emit(Update);
  }
  std::string getChannelName() {return channel_name;}
  bool valid() {
    return Channel.valid();
  }
protected:
  Forwarder::EpicsClient::EpicsClientFactoryInit Initializer; //Must be located before all other members
  epics::pvData::MonitorRequester::shared_pointer monitor_requester;
  epics::pvAccess::ChannelRequester::shared_pointer channel_requester;
  epics::pvAccess::Channel::shared_pointer channel;
  EpicsClientInterface *EpicsClient{nullptr};
  epics::pvAccess::ChannelProvider::shared_pointer provider;
  pvac::ClientChannel Channel;
  pva::Monitor::shared_pointer monitor;
  std::recursive_mutex mx;
  std::string channel_name;
private:
  static std::shared_ptr<pvac::ClientProvider> getClientProvider(std::string ProviderType) {
    static std::map<std::string, std::shared_ptr<pvac::ClientProvider>> ClientProviderMap;
    if (ClientProviderMap.find(ProviderType) == ClientProviderMap.end()) {
      pva::Configuration::shared_pointer EpicsConf(pva::ConfigurationBuilder().push_env().build());
      ClientProviderMap[ProviderType] = std::make_shared<pvac::ClientProvider>(ProviderType, EpicsConf);
    }
    return ClientProviderMap.at(ProviderType);
  }
  SharedLogger Logger = getLogger();
};

} // namespace EpicsClient
} // namespace Forwarder
