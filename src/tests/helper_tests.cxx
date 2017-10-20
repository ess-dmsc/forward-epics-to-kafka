#include "../helper.h"
#include <array>
#include <gtest/gtest.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <string>
#include <vector>
#include <fstream>

TEST(HelperTests, split_with_normal_token) {
  std::vector<std::string> expected = {"hello", " world!"};
  auto actual = split("hello, world!", ",");
  ASSERT_EQ(expected, actual);
}

TEST(HelperTests, split_with_empty_token) {
  std::vector<std::string> expected = {"hello, world!"};
  auto actual = split("hello, world!", "");
  ASSERT_EQ(expected, actual);
}

TEST(HelperTests, split_with_empty_string) {
  std::vector<std::string> expected = {};
  auto actual = split("", ",");
  ASSERT_EQ(expected, actual);
}

TEST(HelperTests, split_with_empty_string_and_token) {
  std::vector<std::string> expected = {""};
  auto actual = split("", "");
  ASSERT_EQ(expected, actual);
}

TEST(HelperTests, split_with_character_not_in_string) {
  std::vector<std::string> expected = {"hello, world!"};
  auto actual = split("hello, world!", "#");
  ASSERT_EQ(expected, actual);
}

TEST(HelperTests, split_with_extended_ascii_token) {
  std::vector<std::string> expected = {"hello, world!"};
  auto actual = split("hello, world!", "╗");
  ASSERT_EQ(expected, actual);
}

TEST(HelperTests, gulp_returns_empty_vector_given_string) {
  std::vector<char> expected = {};
  auto actual = gulp("1");
  ASSERT_EQ(expected, actual);
}

TEST(HelperTests, gulp_returns_correct_binary_when_file_is_read) {
  std::fstream fs("gulptest.txt", std::ios::out | std::ios::binary);
  fs.write("hello", 5);
  fs.close();
  std::vector<char> expected = {'h', 'e', 'l', 'l', 'o'};
  auto actual = gulp("./gulptest.txt");
  ASSERT_EQ(expected, actual);
}

TEST(HelperTests, gulp_returns_empty_vector_when_empty_file_is_given) {
  std::vector<char> expected = {};
  auto actual = gulp("");
  ASSERT_EQ(expected, actual);
}

TEST(HelperTests, get_string_returns_exit_command) {
  std::string expected = "exit";
  rapidjson::Document j0;
  j0.Parse("{\"cmd\": \"exit\"}");

  std::string actual = get_string(&j0, "cmd");

  ASSERT_EQ(expected, actual);
}

TEST(HelperTests, get_string_returns_add_command_with_multiple_streams) {
  std::string expected = "add";
  rapidjson::Document j0;
  j0.Parse("{\n"
           "  \"cmd\": \"add\",\n"
           "  \"streams\": [\n"
           "    {\n"
           "      \"channel\": \"<EPICS PV name>\",\n"
           "      \"converter\": {\n"
           "        \"schema\": \"<schema-id>\",\n"
           "        \"topic\": \"<Kafka-topic>\"\n"
           "      }\n"
           "    },\n"
           "    {\n"
           "      \"channel\": \"<EPICS PV name..>\",\n"
           "      \"converter\": {\n"
           "        \"schema\": \"<schema-id>\",\n"
           "        \"topic\": "
           "\"//<host-if-we-do-not-like-the-default-host>[:port]/"
           "<Kafka-topic..>\"\n"
           "      }\n"
           "    },\n"
           "    {\n"
           "      \"channel\": \"<EPICS Channel Access channel name>\",\n"
           "      \"channel_provider_type\": \"ca\",\n"
           "      \"converter\": {\n"
           "        \"schema\": \"<schema-id>\",\n"
           "        \"topic\": \"<Kafka-topic..>\"\n"
           "      }\n"
           "    }\n"
           "  ]\n"
           "}");

  std::string actual = get_string(&j0, "cmd");

  ASSERT_EQ(expected, actual);
}

TEST(HelperTests, get_string_returns_number_given_number_in_array) {
  std::string expected = "2";
  rapidjson::Document j0;
  j0.Parse("[\"1\", \"2\", \"3\"]");

  std::string actual = get_string(&j0, "1");

  ASSERT_EQ(expected, actual);
}

TEST(HelperTests, get_string_returns_empty_string_if_array_is_empty) {
  std::string expected;
  rapidjson::Document j0;
  j0.Parse("[]");
  std::string actual = get_string(&j0, "1");

  ASSERT_EQ(expected, actual);
}

TEST(HelperTests, get_string_returns_empty_string_if_empty_json) {
  std::string expected;
  rapidjson::Document j0;
  j0.Parse("");
  std::string actual = get_string(&j0, "1");

  ASSERT_EQ(expected, actual);
}

TEST(HelperTests, split_with_empty_string_returns_empty_vector) {
  using std::vector;
  using std::string;
  auto v = split("", "");
  ASSERT_TRUE(v == vector<string>({""}));
}

TEST(HelperTests,
     split_with_no_character_returns_vector_containing_whole_string) {
  using std::vector;
  using std::string;
  auto v = split("abc", "");
  ASSERT_TRUE(v == vector<string>({"abc"}));
}

TEST(
    HelperTests,
    split_returns_vector_with_two_words_in_with_split_character_before_and_after_words) {
  using std::vector;
  using std::string;
  auto v = split("/a/b", "/");
  ASSERT_TRUE(v == vector<string>({"a", "b"}));
}

TEST(
    HelperTests,
    split_does_not_split_all_characters_and_returns_vector_of_words_between_split_character) {
  using std::vector;
  using std::string;
  auto v = split("ac/dc/", "/");
  ASSERT_TRUE(v == vector<string>({"ac", "dc"}));
}

TEST(HelperTests,
     split_adds_no_blank_characters_with_character_before_and_after_string) {
  using std::vector;
  using std::string;
  auto v = split("/ac/dc/", "/");
  ASSERT_TRUE(v == vector<string>({"ac", "dc"}));
}

TEST(HelperTests, split_adds_multiple_words_before_and_after_characters) {
  using std::vector;
  using std::string;
  auto v = split("/some/longer/thing/for/testing", "/");
  ASSERT_TRUE(v ==
              vector<string>({"some", "longer", "thing", "for", "testing"}));
}

TEST(Sleep, sleep_ms) {
  sleep_ms(1);
  // ;-)
}

TEST(RapidTools, get_string_01) {
  using namespace rapidjson;
  Document d;
  d.SetObject();
  auto &a = d.GetAllocator();
  d.AddMember("mem00", Value("s1", a), a);
  Value v2;
  v2.SetObject();
  v2.AddMember("mem10", Value("s2", a), a);
  d.AddMember("mem01", v2.Move(), a);

  {
    Value va;
    va.SetArray();
    va.PushBack(Value("something_a_0", a), a);
    va.PushBack(Value("something_a_1", a), a);
    va.PushBack(Value(1234), a);
    d.AddMember("mem02", va, a);
  }

  StringBuffer buf1;
  PrettyWriter<StringBuffer> wr(buf1);
  d.Accept(wr);
  auto s1 = get_string(&d, "mem00");
  ASSERT_EQ(s1, "s1");
  s1 = get_string(&d, "mem01.mem10");
  ASSERT_EQ(s1, "s2");
  s1 = get_string(&d, "mem02.1");
  ASSERT_EQ(s1, "something_a_1");
}
