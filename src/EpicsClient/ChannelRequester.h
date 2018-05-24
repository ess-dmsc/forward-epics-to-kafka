#include <pv/pvAccess.h>
#include <pv/pvData.h>
#include <string>

namespace BrightnESS {
namespace ForwardEpicsToKafka {
namespace EpicsClient {

char const *channel_state_name(epics::pvAccess::Channel::ConnectionState x);

class EpicsClientMonitor_impl;

class ChannelRequester : public epics::pvAccess::ChannelRequester {
public:
  ChannelRequester(EpicsClientMonitor_impl *epics_client_impl);
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
  EpicsClientMonitor_impl *epics_client_impl = nullptr;
};
}
}
}
