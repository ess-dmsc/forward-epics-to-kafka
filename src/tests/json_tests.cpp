#include "../json.h"
#include "../logger.h"
#include <gtest/gtest.h>

TEST(JsonTest, JSON_integer_maximum_values) {
  using nlohmann::json;
  auto Doc = json::parse(R"""({
    "large_uint32": 4294967295,
    "large_int32_pos": 2147483647,
    "large_int32_neg": -2147483648,
    "large_uint64": 18446744073709551615,
    "large_int64_pos": 9223372036854775807,
    "large_int64_neg": -9223372036854775808
  })""");
  ASSERT_TRUE(Doc.is_object());

  ASSERT_TRUE(find<uint32_t>("large_uint32", Doc));
  ASSERT_TRUE(find<int32_t>("large_int32_pos", Doc));
  ASSERT_TRUE(find<int32_t>("large_int32_neg", Doc));
  ASSERT_EQ(0xffffffffu, find<uint32_t>("large_uint32", Doc).inner());
  ASSERT_EQ(+0x7fffffff, find<int32_t>("large_int32_pos", Doc).inner());
  ASSERT_EQ(int32_t(-0x80000000),
            find<int32_t>("large_int32_neg", Doc).inner());

  ASSERT_TRUE(find<uint64_t>("large_uint64", Doc));
  ASSERT_TRUE(find<int64_t>("large_int64_pos", Doc));
  ASSERT_TRUE(find<int64_t>("large_int64_neg", Doc));
  ASSERT_EQ(0xffffffffffffffffull, find<uint64_t>("large_uint64", Doc).inner());
  ASSERT_EQ(+0x7fffffffffffffffll,
            find<int64_t>("large_int64_pos", Doc).inner());
  ASSERT_EQ(int64_t(-0x8000000000000000ll),
            find<int64_t>("large_int64_neg", Doc).inner());
}

TEST(JsonTest, given_JSON_array_verify_it_finds_JSON_array_type) {
  using nlohmann::json;
  auto Doc = json::parse(R"""({
    "some_array": [1, 2, 3]
  })""");
  ASSERT_TRUE(Doc.is_object());
  auto Maybe = find_array("some_array", Doc);
  ASSERT_TRUE(Maybe);
  ASSERT_TRUE(Maybe.inner().is_array());
}

TEST(JsonTest, iterate_object) {
  using nlohmann::json;
  auto Doc = json::parse(R"""({
    "some_array": [1, 2, 3],
    "some_object": {},
    "some_number": 123
  })""");
  ASSERT_TRUE(Doc.is_object());
  size_t Count = 0;
  for (auto It = Doc.begin(); It != Doc.end(); ++It) {
    Count += 1;
  }
  ASSERT_EQ(Count, 3u);
}

TEST(JsonTest, accessing_missing_key_should_throw) {
  using nlohmann::json;
  auto Doc = json::parse("{}");
  ASSERT_TRUE(Doc.is_object());
  try {
    find<int>("does_not_exist", Doc).inner();
    ASSERT_EQ("Accessing missing key did not throw.", "");
  } catch (...) {
  }
}
