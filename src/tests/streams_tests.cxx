#include "../Stream.h"
#include "../Streams.h"
#include "../helper.h"
#include <gmock/gmock.h>

using namespace testing;
using namespace BrightnESS::ForwardEpicsToKafka;

class MockStream : public Stream {
public:
  MockStream(ChannelInfo channel_info) : Stream(channel_info) {}
};

std::shared_ptr<MockStream> createMockStream(std::string provider_type,
                                             std::string channel_name) {
  ChannelInfo ci{provider_type, channel_name};
  return std::make_shared<MockStream>(ci);
}

TEST(StreamsTest, streams_are_empty_on_initialisation) {
  Streams streams;
  ASSERT_EQ(streams.size(), 0);
}

TEST(StreamsTest, stream_size_is_one_when_stream_is_added) {
  Streams streams;
  streams.add(createMockStream("hello", "world"));
  ASSERT_EQ(1, streams.size());
}

TEST(StreamsTest, stream_size_is_two_when_two_streams_are_added) {
  Streams streams;
  streams.add(createMockStream("hello", "world"));
  streams.add(createMockStream("world", "hello"));
  ASSERT_EQ(2, streams.size());
}

TEST(StreamsTest, stream_size_is_zero_when_clear_is_called) {
  Streams streams;
  streams.add(createMockStream("hello", "world"));
  streams.add(createMockStream("world", "hello"));
  streams.streams_clear();
  ASSERT_EQ(0, streams.size());
}

TEST(StreamsTest, stream_size_is_one_stream_is_added_after_clear) {
  Streams streams;
  streams.add(createMockStream("hello", "world"));
  streams.add(createMockStream("world", "hello"));
  streams.streams_clear();
  streams.add(createMockStream("hello", "hello"));
  ASSERT_EQ(1, streams.size());
}

TEST(StreamsTest, back_returns_correct_stream_when_streams_are_added) {
  Streams streams;
  streams.add(createMockStream("hello", "world"));
  streams.add(createMockStream("world", "hello"));
  std::shared_ptr<MockStream> x = createMockStream("hello", "hello");
  streams.add(x);
  ASSERT_EQ(x.get()->channel_info().channel_name,
            streams.back()->channel_info().channel_name);
}

TEST(StreamsTest,
     back_returns_nullptr_when_streams_is_empty_after_initialisation) {
  Streams streams;
  ASSERT_EQ(streams.back(), nullptr);
}

TEST(
    StreamsTest,
    back_throws_range_error_when_streams_is_empty_after_clear_streams_is_called) {
  Streams streams;
  streams.add(createMockStream("hello", "world"));
  streams.add(createMockStream("world", "hello"));
  streams.streams_clear();
  ASSERT_EQ(streams.back(), nullptr);
}

TEST(StreamsTest, check_stream_status_on_streams_after_clear_does_nothing) {
  Streams streams;
  streams.add(createMockStream("hello", "world"));
  streams.add(createMockStream("world", "hello"));
  streams.streams_clear();
  streams.check_stream_status();
  ASSERT_EQ(nullptr, streams.back().get());
}

TEST(StreamsTest,
     check_stream_status_on_one_stream_with_negative_status_removes_stream) {
  Streams streams;
  auto s = createMockStream("hello", "world");
  s.get()->error_in_epics(); // sets status to -1
  streams.add(s);
  streams.check_stream_status();
  ASSERT_EQ(nullptr, streams.back().get());
}

TEST(
    StreamsTest,
    check_stream_status_with_multiple_streams_with_negative_statuses_removes_all_streams) {
  Streams streams;

  auto s = createMockStream("hello", "world");
  s.get()->error_in_epics(); // sets status to -1
  auto s2 = createMockStream("world", "world");
  s2.get()->error_in_epics(); // sets status to -1
  streams.add(s);
  streams.add(s2);
  streams.check_stream_status();
  ASSERT_EQ(nullptr, streams.back().get());
  ASSERT_EQ(0, streams.size());
}

TEST(
    StreamsTest,
    check_stream_status_with_multiple_streams_with_negative_statuses_removes_all_streams_and_leaves_positive_status_streams) {
  Streams streams;

  auto s = createMockStream("hello", "world");
  s.get()->error_in_epics(); // sets status to -1
  auto s2 = createMockStream("world", "world");
  streams.add(s);
  streams.add(s2);
  streams.check_stream_status();
  ASSERT_EQ(s2.get(), streams.back().get());
  ASSERT_EQ(1, streams.size());
}

TEST(StreamsTest,
     channel_stop_removes_single_channel_with_matched_channel_name) {
  Streams streams;
  auto s = createMockStream("hello", "world");
  streams.add(s);
  streams.channel_stop("world");
  ASSERT_EQ(nullptr, streams.back().get());
  ASSERT_EQ(0, streams.size());
}

TEST(StreamsTest, channel_stop_removes_all_channels_with_matched_channel_name) {
  Streams streams;

  auto s = createMockStream("hello", "world");
  auto s2 = createMockStream("world", "world");

  streams.add(s);
  streams.add(s2);
  streams.channel_stop("world");
  ASSERT_EQ(nullptr, streams.back().get());
  ASSERT_EQ(0, streams.size());
}

TEST(StreamsTest,
     channel_stop_removes_no_channels_with_no_matched_channel_name) {
  Streams streams;
  auto s = createMockStream("hello", "world");
  auto s2 = createMockStream("world", "world");
  streams.add(s);
  streams.add(s2);
  streams.channel_stop("nothelloworld");
  ASSERT_EQ(s2.get(), streams.back().get());
  ASSERT_EQ(2, streams.size());
}

TEST(StreamsTest,
     channel_stop_removes_no_channels_with_no__given_channel_name) {
  Streams streams;
  auto s = createMockStream("hello", "world");
  auto s2 = createMockStream("world", "world");
  streams.add(s);
  streams.add(s2);
  streams.channel_stop("");
  ASSERT_EQ(s2.get(), streams.back().get());
  ASSERT_EQ(2, streams.size());
}
