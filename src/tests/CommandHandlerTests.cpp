// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

#include <CommandHandler.h>
#include <ConfigParser.h>
#include <Forwarder.h>
#include <MainOpt.h>
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

class CommandHandlerTest : public ::testing::Test {
protected:
  Forwarder::MainOpt MainOpt;
  Forwarder::Forwarder Main{MainOpt};
  Forwarder::ConfigCB Config{Main};
};

TEST_F(CommandHandlerTest, add_command_adds_stream_correctly) {
  std::string RawJson = R"({
                            "cmd": "add",
                            "streams": [
                              {
                                "channel": "my_channel_name",
                                "channel_provider_type": "ca"
                              }
                            ]
                           })";

  Config(RawJson);

  ASSERT_EQ(1u, Main.streams.size());
  ASSERT_EQ("my_channel_name", Main.streams[0]->getChannelInfo().channel_name);
  ASSERT_EQ("ca", Main.streams[0]->getChannelInfo().provider_type);
}

TEST_F(CommandHandlerTest, adding_stream_twice_ignores_second) {
  std::string RawJson1 = R"({
                            "cmd": "add",
                            "streams": [
                              {
                                "channel": "my_channel_name",
                                "channel_provider_type": "ca"
                              }
                            ]
                           })";

  // Changed the channel_provider_type as it gives us something to test for
  std::string RawJson2 = R"({
                            "cmd": "add",
                            "streams": [
                              {
                                "channel": "my_channel_name",
                                "channel_provider_type": "pva"
                              }
                            ]
                           })";

  Config(RawJson1);
  Config(RawJson2);

  ASSERT_EQ(1u, Main.streams.size());
  ASSERT_EQ("my_channel_name", Main.streams[0]->getChannelInfo().channel_name);
  // The second command should not cause the provider type to change
  ASSERT_EQ("ca", Main.streams[0]->getChannelInfo().provider_type);
}

TEST_F(CommandHandlerTest, add_command_adds_multiple_streams_correctly) {
  std::string RawJson = R"({
                            "cmd": "add",
                            "streams": [
                              {
                              "channel": "my_channel_name",
                              "channel_provider_type": "ca"
                              },
                              {
                                "channel": "my_channel_name_2",
                                "channel_provider_type": "pva"
                              }
                            ]
                           })";

  Config(RawJson);

  ASSERT_EQ(2u, Main.streams.size());
  ASSERT_EQ("my_channel_name", Main.streams[0]->getChannelInfo().channel_name);
  ASSERT_EQ("ca", Main.streams[0]->getChannelInfo().provider_type);
  ASSERT_EQ("my_channel_name_2",
            Main.streams[1]->getChannelInfo().channel_name);
  ASSERT_EQ("pva", Main.streams[1]->getChannelInfo().provider_type);
}

TEST_F(CommandHandlerTest, stop_all_command_removes_all_streams_correctly) {
  std::string AddJson = R"({
                            "cmd": "add",
                            "streams": [
                              {
                                "channel": "my_channel_name",
                                "channel_provider_type": "ca"
                              },
                              {
                                "channel": "my_channel_name_2",
                                "channel_provider_type": "pva"
                              }
                            ]
                           })";

  Config(AddJson);

  std::string RemoveJson = R"({
                               "cmd": "stop_all"
                              })";

  Config(RemoveJson);

  ASSERT_EQ(0u, Main.streams.size());
}

TEST_F(CommandHandlerTest, stop_command_removes_stream_correctly) {
  std::string AddJson = R"({
                            "cmd": "add",
                            "streams": [
                              {
                                "channel": "my_channel_name",
                                "channel_provider_type": "ca"
                              }
                            ]
                           })";
  Config(AddJson);

  std::string RemoveJson = R"({
                               "cmd": "stop_channel",
                               "channel": "my_channel_name"
                              })";

  Config(RemoveJson);

  ASSERT_EQ(0u, Main.streams.size());
}

class ExtractCommandsTest : public ::testing::TestWithParam<const char *> {
  // cppcheck-suppress unusedFunction
  void SetUp() override { command = (*GetParam()); }

protected:
  std::string command;
};

TEST_P(ExtractCommandsTest, extracting_command_gets_command_name) {
  std::ostringstream os;
  os << "{"
     << R"(  "cmd": ")" << command << "\""
     << "}";

  std::string RawJson = os.str();

  nlohmann::json Json = nlohmann::json::parse(RawJson);

  auto Cmd = Forwarder::ConfigCB::findCommand(Json);

  ASSERT_EQ(command, Cmd);
}

INSTANTIATE_TEST_CASE_P(InstantiationName, ExtractCommandsTest,
                        ::testing::Values("add", "stop_channel", "stop_all",
                                          "exit", "unknown_command"));
