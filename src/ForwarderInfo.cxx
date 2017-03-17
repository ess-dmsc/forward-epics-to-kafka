#include "ForwarderInfo.h"
#include "Main.h"

namespace BrightnESS {
namespace ForwardEpicsToKafka {

ForwarderInfo::ForwarderInfo(Main * main) : main(main) {
}

ForwarderInfo::~ForwarderInfo() {
	main = nullptr;
}

}
}
