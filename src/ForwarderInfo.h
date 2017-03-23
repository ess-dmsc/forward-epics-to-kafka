#pragma once

#include <cstdint>

namespace BrightnESS {
namespace ForwardEpicsToKafka {

class Main;

/**
Only for internal testing.
*/
class ForwarderInfo {
public:
ForwarderInfo(Main * main);
~ForwarderInfo();
uint64_t fwdix = 0;
uint64_t teamid = 0;
private:
Main * main = nullptr;
};

}
}
