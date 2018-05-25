#pragma once

#include <cstdint>

namespace BrightnESS {
namespace ForwardEpicsToKafka {

class Forwarder;

/**
Only for internal testing.
*/
class ForwarderInfo {
public:
  ForwarderInfo(Forwarder *main);
  ~ForwarderInfo();
  uint64_t fwdix = 0;
  uint64_t teamid = 0;

private:
  Forwarder *main = nullptr;
};
}
}
