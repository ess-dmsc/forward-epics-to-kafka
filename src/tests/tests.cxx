#include "tests.h"
#include <gtest/gtest.h>
#include "../MainOpt.h"

namespace BrightnESS {
namespace ForwardEpicsToKafka {
MainOpt * Tests::main_opt = nullptr;
}
}

int main(int argc, char ** argv) {
	auto po = BrightnESS::ForwardEpicsToKafka::parse_opt(argc, argv);
	if (po.first) {
		return 1;
	}
	auto opt = std::move(po.second);
	BrightnESS::ForwardEpicsToKafka::Tests::main_opt = opt.get();
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
