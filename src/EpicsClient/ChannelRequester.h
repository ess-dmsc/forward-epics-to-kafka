#include <pv/pvAccess.h>
#include <pv/pvData.h>
#include <string>

namespace Forwarder {
namespace EpicsClient {

class EpicsClient_impl;

class ChannelRequester : public epics::pvAccess::ChannelRequester {
public:
  ChannelRequester(EpicsClient_impl *epics_client_impl);
  // From class pvData::Requester
  std::string getRequesterName() override;
  void message(std::string const &message,
               epics::pvData::MessageType messageType) override;
  void channelCreated(
      const epics::pvData::Status &status,
      epics::pvAccess::Channel::shared_pointer const &channel) override;
  void channelStateChange(
      epics::pvAccess::Channel::shared_pointer const &channel,
      epics::pvAccess::Channel::ConnectionState connectionState) override;

private:
  epics::pvData::MonitorRequester::shared_pointer monitor_requester;
  epics::pvData::MonitorPtr monitor;
  EpicsClient_impl *epics_client_impl = nullptr;
};
}
}
