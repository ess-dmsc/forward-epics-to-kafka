#include <gtest/gtest.h>
#include "../logger.h"
#include "../MainOpt.h"

using namespace BrightnESS::ForwardEpicsToKafka;

TEST(MainOpt, parse_config_file) {
	MainOpt opt;

	// next test is expected to fail
	auto ll = log_level;
	log_level = 9;
	opt.parse_json_file("tests/test-config-valid.json");
	log_level = ll;
}
