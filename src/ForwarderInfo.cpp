#include "ForwarderInfo.h"
#include "Forwarder.h"

namespace Forwarder {

ForwarderInfo::ForwarderInfo(Forwarder *main) : main(main) {}

ForwarderInfo::~ForwarderInfo() { main = nullptr; }
}
