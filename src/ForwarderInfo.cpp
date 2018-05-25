#include "ForwarderInfo.h"
#include "Forwarder.h"

namespace BrightnESS {
namespace ForwardEpicsToKafka {

ForwarderInfo::ForwarderInfo(Forwarder *main) : main(main) {}

ForwarderInfo::~ForwarderInfo() { main = nullptr; }
}
}
