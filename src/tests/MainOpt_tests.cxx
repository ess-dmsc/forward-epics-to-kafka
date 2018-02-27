#include "../MainOpt.h"
#include "../logger.h"
#include <gtest/gtest.h>

using namespace BrightnESS::ForwardEpicsToKafka;

class MainOpt_T : public testing::Test {
public:
};

TEST(MainOpt_T, parse_config_file) {
  MainOpt opt;

  ASSERT_EQ(opt.parse_json_file("./tests/test-config-valid.json"), 0);
  ASSERT_TRUE(opt.brokers.size() == 2);
  ASSERT_EQ(opt.brokers.at(0).host_port, "localhost:9092");
  ASSERT_EQ(opt.brokers.at(1).host_port, "127.0.0.1:9092");

  // next test is expected to fail
  auto ll = log_level;
  log_level = 0;
  ASSERT_LT(opt.parse_json_file("./tests/test-config-invalid.json"), 0);
  log_level = ll;
}

TEST(MainOpt_T, test_find_broker_returns_correct_broker) {
  MainOpt opt;

  const char *json = R"({ "broker" : "localhost:9002" })";
  opt.json = std::make_shared<rapidjson::Document>();
  opt.json->Parse(json);
  opt.find_broker();

  ASSERT_EQ(opt.brokers.at(0).host, "localhost");
  ASSERT_EQ(opt.brokers.at(0).host_port, "localhost:9002");
}

TEST(MainOpt_T, test_find_broker_with_no_broker_returns_default_string) {
  MainOpt opt;

  const char *json = R"({
      "main-poll-interval" : 3
      })";
  opt.json = std::make_shared<rapidjson::Document>();
  opt.json->Parse(json);
  opt.find_broker();
  ASSERT_EQ(opt.brokers.at(0).host, "localhost");
  ASSERT_EQ(opt.brokers.at(0).host_port, "localhost:9092");
}

TEST(MainOpt_T, test_find_conversion_threads_returns_correct_number) {
  MainOpt opt;

  const char *json = R"({ "conversion-threads" : 4 })";
  opt.json = std::make_shared<rapidjson::Document>();
  opt.json->Parse(json);
  opt.find_conversion_threads(opt.conversion_threads);
  ASSERT_EQ(opt.conversion_threads, 4);
}

TEST(MainOpt_T,
     test_find_conversion_threads_returns_default_if_no_property_found) {
  MainOpt opt;

  const char *json = R"({ "broker" : "localhost:9003" })";
  opt.json = std::make_shared<rapidjson::Document>();
  opt.json->Parse(json);
  opt.find_conversion_threads(opt.conversion_threads);
  ASSERT_EQ(opt.conversion_threads, 1);
}

TEST(MainOpt_T,
     test_find_broker_returns_correct_broker_after_other_properties_found) {
  MainOpt opt;

  const char *json =
      R"({ "broker" : "localhost:9002", "conversion-threads" : 4 })";
  opt.json = std::make_shared<rapidjson::Document>();
  opt.json->Parse(json);
  opt.find_conversion_threads(opt.conversion_threads);
  ASSERT_EQ(opt.conversion_threads, 4);
  opt.find_broker();
  ASSERT_EQ(opt.brokers.at(0).host, "localhost");
  ASSERT_EQ(opt.brokers.at(0).host_port, "localhost:9002");
}

TEST(MainOpt_T, test_find_brokers_config_finds_string_property) {
  MainOpt opt;
  const char *json = R"({
        "kafka": {
          "broker": {
            "hello" : "world"
          }
        }
      })";
  opt.json = std::make_shared<rapidjson::Document>();
  opt.json->Parse(json);
  opt.find_brokers_config();
  ASSERT_EQ(opt.broker_opt.conf_strings["hello"], "world");
}

TEST(MainOpt_T, test_find_brokers_config_finds_int_property) {
  MainOpt opt;
  const char *json = R"({
        "kafka": {
          "broker": {
            "hello" : 50
          }
        }
      })";
  opt.json = std::make_shared<rapidjson::Document>();
  opt.json->Parse(json);
  opt.find_brokers_config();
  ASSERT_EQ(opt.broker_opt.conf_ints["hello"], 50);
}

TEST(MainOpt_T, test_find_brokers_config_does_nothing_with_object_property) {
  MainOpt opt;
  const char *json = R"({
        "kafka": {
          "broker": {
            "hello" : {}
          }
        }
      })";
  opt.json = std::make_shared<rapidjson::Document>();
  opt.json->Parse(json);
  opt.find_brokers_config();

  ASSERT_TRUE(opt.broker_opt.conf_strings.find("hello") ==
              opt.broker_opt.conf_strings.end());
  ASSERT_TRUE(opt.broker_opt.conf_ints.find("hello") ==
              opt.broker_opt.conf_ints.end());
}

TEST(MainOpt_T, test_find_main_poll_interval_returns_correct_value) {
  MainOpt opt;
  const char *json = R"({
      "main-poll-interval" : 3
      })";
  opt.json = std::make_shared<rapidjson::Document>();
  opt.json->Parse(json);
  opt.find_main_poll_interval(opt.main_poll_interval);
  ASSERT_EQ(opt.main_poll_interval, 3);
}

TEST(MainOpt_T, test_find_conversion_threads_returns_correct_value) {
  MainOpt opt;
  const char *json = R"({
      "conversion-threads":3
      })";
  opt.json = std::make_shared<rapidjson::Document>();
  opt.json->Parse(json);
  opt.find_conversion_threads(opt.conversion_threads);
  ASSERT_EQ(opt.conversion_threads, 3);
}

TEST(MainOpt_T, test_find_conversion_worker_queue_size_returns_correct_value) {
  MainOpt opt;
  const char *json = R"({
      "conversion-worker-queue-size" : 3
      })";
  opt.json = std::make_shared<rapidjson::Document>();
  opt.json->Parse(json);
  opt.find_conversion_worker_queue_size(opt.conversion_worker_queue_size);
  ASSERT_EQ(opt.conversion_worker_queue_size, 3);
}
