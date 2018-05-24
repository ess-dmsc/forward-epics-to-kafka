#pragma once
#include "EpicsClientInterface.h"
#include "Ring.h"
#include <memory>

namespace BrightnESS {
namespace ForwardEpicsToKafka {
namespace EpicsClient {
class Stream;

class EpicsClientPeriodic : public EpicsClient::EpicsClientInterface {
public:
  Stream *stream = nullptr;
  EpicsClientPeriodic(
      int period, std::string channelName,
      std::string epics_channel_provider_type,
      std::shared_ptr<Ring<std::unique_ptr<FlatBufs::EpicsPVUpdate>>> ring);
  int emit(std::unique_ptr<FlatBufs::EpicsPVUpdate> up) override;
  int stop() override { return 1; };
  void error_in_epics() override{};
  int status() override { return 1; };

private:
  //  epics::pvAccess::Channel::shared_pointer channel;
  std::shared_ptr<Ring<std::unique_ptr<FlatBufs::EpicsPVUpdate>>> emit_queue;
  std::string channel_name;

  //  epics::pvaClient::PvaClientGet;
};
}
}
}