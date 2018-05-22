#include "../CommandHandler.h"
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

class ExtractCommandsTest : public ::testing::TestWithParam<const char *> {
  virtual void SetUp() { command = (*GetParam()); }
  virtual void TearDown() {}

protected:
  std::string command;
};

TEST_P(ExtractCommandsTest, extracting_command_gets_command_name) {
  std::ostringstream os;
  os << "{"
     << "  \"cmd\": \"" << command << "\""
     << "}";

  std::string RawJson = os.str();

  nlohmann::json Json = nlohmann::json::parse(RawJson);
  BrightnESS::ForwardEpicsToKafka::MainOpt MainOpt;
  BrightnESS::ForwardEpicsToKafka::Main Main(MainOpt);
  BrightnESS::ForwardEpicsToKafka::ConfigCB config(Main);

  auto Cmd = config.findCommand(Json);

  ASSERT_EQ(command, Cmd);
}

INSTANTIATE_TEST_CASE_P(InstantiationName, ExtractCommandsTest,
                        ::testing::Values("add", "stop_channel", "stop_all",
                                          "exit", "unknown_command"));
