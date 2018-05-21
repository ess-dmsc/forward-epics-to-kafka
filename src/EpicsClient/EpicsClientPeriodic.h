#pragma once
#include <pv/pvaClient.h>

namespace BrightnESS {
namespace ForwardEpicsToKafka {
namespace EpicsClient {
class Stream;

class EpicsClientPeriodic {
public:
  Stream *stream = nullptr;
  EpicsClientPeriodic(Stream *stream, std::string epics_channel_provider_type,
                      std::string channel_name);

private:
  //  epics::pvaClient::PvaClientGet;
};
}
}
}