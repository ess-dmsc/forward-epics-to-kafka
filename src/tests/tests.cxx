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
	std::string f = ::testing::GTEST_FLAG(filter);
	if (f.find("Remote") == std::string::npos) {
		f = f + std::string(":-*Remote*");
	}
	::testing::GTEST_FLAG(filter) = f;
	return RUN_ALL_TESTS();
}
