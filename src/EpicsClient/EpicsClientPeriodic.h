#pragma once
#include "EpicsClientInterface.h"
#include "Ring.h"
#include <memory>

namespace BrightnESS {
namespace ForwardEpicsToKafka {
struct ChannelInfo;
namespace EpicsClient {
class Stream;


///\class EpicsClientPeriodic
///\brief epics client wrapper responsible for periodically checking PV values
class EpicsClientPeriodic : public EpicsClient::EpicsClientInterface {
public:
  Stream *stream = nullptr;
  EpicsClientPeriodic(
      ChannelInfo &channelInfo,
      std::shared_ptr<Ring<std::unique_ptr<FlatBufs::EpicsPVUpdate>>> ring);
  int emit(std::unique_ptr<FlatBufs::EpicsPVUpdate> up) override;
  int stop() override { return 1; };
  void error_in_epics() override{};
  int status() override { return 1; };

private:
  std::shared_ptr<Ring<std::unique_ptr<FlatBufs::EpicsPVUpdate>>> emit_queue;
  std::string channel_name;
};
}
}
}