#include "../helper.h"
#include <array>
#include <fstream>
#include <gtest/gtest.h>
#include <string>
#include <vector>

TEST(HelperTest, split_with_normal_token) {
  std::vector<std::string> expected = {"hello", " world!"};
  auto actual = split("hello, world!", ",");
  ASSERT_EQ(expected, actual);
}

TEST(HelperTest, split_with_empty_token) {
  std::vector<std::string> expected = {"hello, world!"};
  auto actual = split("hello, world!", "");
  ASSERT_EQ(expected, actual);
}

TEST(HelperTest, split_with_empty_string) {
  auto actual = split("", ",");
  ASSERT_TRUE(actual.empty());
}

TEST(HelperTest, split_with_empty_string_and_token) {
  auto actual = split("", "");
  ASSERT_TRUE(actual.empty());
}

TEST(HelperTest, split_with_character_not_in_string) {
  std::vector<std::string> expected = {"hello, world!"};
  auto actual = split("hello, world!", "#");
  ASSERT_EQ(expected, actual);
}

TEST(HelperTest, split_with_extended_ascii_token) {
  std::vector<std::string> expected = {"hello, world!"};
  auto actual = split("hello, world!", "╗");
  ASSERT_EQ(expected, actual);
}

TEST(HelperTest, gulp_returns_empty_vector_given_string) {
  auto actual = readFile("1");
  ASSERT_TRUE(actual.empty());
}

TEST(HelperTest, gulp_returns_correct_binary_when_file_is_read) {
  std::fstream fs("gulptest.txt", std::ios::out | std::ios::binary);
  fs.write("hello", 5);
  fs.close();
  std::vector<char> expected = {'h', 'e', 'l', 'l', 'o'};
  auto actual = readFile("./gulptest.txt");
  ASSERT_EQ(expected, actual);
}

TEST(HelperTest, gulp_returns_empty_vector_when_empty_file_is_given) {
  auto actual = readFile("");
  ASSERT_TRUE(actual.empty());
}

TEST(HelperTest, split_with_empty_string_returns_empty_vector) {
  auto v = split("", "");
  ASSERT_TRUE(v.empty());
}

TEST(HelperTest,
     split_with_no_character_returns_vector_containing_whole_string) {
  auto v = split("abc", "");
  ASSERT_EQ(v, std::vector<std::string>({"abc"}));
}

TEST(
    HelperTest,
    split_returns_vector_with_two_words_in_with_split_character_before_and_after_words) {
  auto v = split("/a/b", "/");
  ASSERT_EQ(v, std::vector<std::string>({"a", "b"}));
}

TEST(
    HelperTest,
    split_does_not_split_all_characters_and_returns_vector_of_words_between_split_character) {
  auto v = split("ac/dc/", "/");
  ASSERT_EQ(v, std::vector<std::string>({"ac", "dc"}));
}

TEST(HelperTest,
     split_adds_no_blank_characters_with_character_before_and_after_string) {
  auto v = split("/ac/dc/", "/");
  ASSERT_EQ(v, std::vector<std::string>({"ac", "dc"}));
}

TEST(HelperTest, split_adds_multiple_words_before_and_after_characters) {
  auto v = split("/some/longer/thing/for/testing", "/");
  ASSERT_EQ(v, std::vector<std::string>(
                   {"some", "longer", "thing", "for", "testing"}));
}

TEST(Sleep, sleep_ms) {
  sleep_ms(1);
  // ;-)
}
