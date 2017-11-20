#include "../MainOpt.h"
#include "../logger.h"
#include <gtest/gtest.h>

using namespace BrightnESS::ForwardEpicsToKafka;

class MainOpt_T : public testing::Test {
public:
};

//TEST_F(MainOpt_T, parse_config_file) {
//  MainOpt opt;
//
//  ASSERT_EQ(opt.parse_json_file("tests/test-config-valid.json"), 0);
//  ASSERT_TRUE(opt.brokers.size() == 2);
//  ASSERT_EQ(opt.brokers.at(0).host_port, "localhost:9092");
//  ASSERT_EQ(opt.brokers.at(1).host_port, "127.0.0.1:9092");
//
//  // next test is expected to fail
//  auto ll = log_level;
//  log_level = 0;
//  ASSERT_LT(opt.parse_json_file("tests/test-config-invalid.json"), 0);
//  log_level = ll;
//}

TEST_F(MainOpt_T, test_find_broker_returns_correct_broker) {
  MainOpt opt;

  const char* json = "{ \"broker\" : \"localhost:9002\" }";
  rapidjson::Document document;
  document.Parse(json);

  ASSERT_EQ(opt.find_broker(document), "localhost:9002");
}

TEST_F(MainOpt_T, test_find_broker_with_no_broker_returns_empty_string){
  MainOpt opt;

  const char* json = "{}";
  rapidjson::Document document;
  document.Parse(json);

  ASSERT_EQ(opt.find_broker(document), "");
}


TEST_F(MainOpt_T, test_find_conversion_threads_returns_correct_number) {
  MainOpt opt;

  const char* json = "{ \"conversion-threads\" : 4 }";
  rapidjson::Document document;
  document.Parse(json);

  ASSERT_EQ(opt.find_conversion_threads(document), 4);
}

TEST_F(MainOpt_T, test_find_conversion_threads_returns_zero_if_no_property_found) {
  MainOpt opt;

  const char* json = "{ \"broker\" : \"localhost:9003\" }";
  rapidjson::Document document;
  document.Parse(json);

  ASSERT_EQ(opt.find_conversion_threads(document), 0);
}

TEST_F(MainOpt_T, test_find_broker_returns_correct_broker_after_other_properties_found) {
  MainOpt opt;

  const char* json = "{ \"broker\" : \"localhost:9002\", \"conversion-threads\" : 4 }";
  rapidjson::Document document;
  document.Parse(json);
  ASSERT_EQ(opt.find_conversion_threads(document), 4);
  ASSERT_EQ(opt.find_broker(document), "localhost:9002");
}