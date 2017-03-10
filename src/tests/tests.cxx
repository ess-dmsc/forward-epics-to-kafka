#include <gtest/gtest.h>
#include "../MainOpt.h"

int main(int argc, char ** argv) {
	auto po = BrightnESS::ForwardEpicsToKafka::parse_opt(argc, argv);
	if (po.first) {
		return 1;
	}
	auto opt = std::move(po.second);
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
