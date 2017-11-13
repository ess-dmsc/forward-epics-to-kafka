#include "../MainOpt.h"
#include "../logger.h"
#include <gtest/gtest.h>

using namespace BrightnESS::ForwardEpicsToKafka;

class MainOpt_T : public testing::Test {
public:
};

TEST_F(MainOpt_T, parse_config_file) {
  MainOpt opt;

  ASSERT_EQ(opt.parse_json_file("tests/test-config-valid.json"), 0);
  ASSERT_TRUE(opt.brokers.size() == 2);
  ASSERT_EQ(opt.brokers.at(0).host_port, "localhost:9092");
  ASSERT_EQ(opt.brokers.at(1).host_port, "127.0.0.1:9092");

  // next test is expected to fail
  auto ll = log_level;
  log_level = 0;
  ASSERT_LT(opt.parse_json_file("tests/test-config-invalid.json"), 0);
  log_level = ll;
}
