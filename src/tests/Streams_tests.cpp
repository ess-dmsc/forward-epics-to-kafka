#include "../Stream.h"
#include "../Streams.h"
#include "../helper.h"
#include <gmock/gmock.h>

using namespace testing;
using namespace Forwarder;

class FakeEpicsClient : public EpicsClient::EpicsClientInterface {
public:
  FakeEpicsClient() = default;
  int emit(std::shared_ptr<FlatBufs::EpicsPVUpdate> Update) override {
    return 0;
  };
  int stop() override { return 0; };
  void errorInEpics() override { status_ = -1; };
  int status() override { return status_; };

private:
  int status_{0};
};

std::shared_ptr<Stream> createStream(std::string ProviderType,
                                     std::string ChannelName) {
  auto ring = std::make_shared<
      moodycamel::ConcurrentQueue<std::shared_ptr<FlatBufs::EpicsPVUpdate>>>();
  auto client = make_unique<FakeEpicsClient>();
  ChannelInfo ci{std::move(ProviderType), std::move(ChannelName)};
  return std::make_shared<Stream>(ci, std::move(client), ring);
}

TEST(StreamsTest, streams_are_empty_on_initialisation) {
  Streams streams;
  ASSERT_EQ(streams.size(), 0u);
}

TEST(StreamsTest, stream_size_is_one_when_stream_is_added) {
  Streams streams;
  streams.add(createStream("hello", "world"));
  ASSERT_EQ(streams.size(), 1u);
}

TEST(StreamsTest, stream_size_is_two_when_two_streams_are_added) {
  Streams streams;
  streams.add(createStream("hello", "world"));
  streams.add(createStream("world", "hello"));
  ASSERT_EQ(streams.size(), 2u);
}

TEST(StreamsTest, stream_size_is_zero_when_clear_is_called) {
  Streams streams;
  streams.add(createStream("hello", "world"));
  streams.add(createStream("world", "hello"));
  streams.clearStreams();
  ASSERT_EQ(streams.size(), 0u);
}

TEST(StreamsTest, stream_size_is_one_stream_is_added_after_clear) {
  Streams streams;
  streams.add(createStream("hello", "world"));
  streams.add(createStream("world", "hello"));
  streams.clearStreams();
  streams.add(createStream("hello", "hello"));
  ASSERT_EQ(streams.size(), 1u);
}

TEST(StreamsTest, back_returns_correct_stream_when_streams_are_added) {
  Streams streams;
  streams.add(createStream("hello", "world"));
  streams.add(createStream("world", "hello"));
  std::shared_ptr<Stream> x = createStream("hello", "hello");
  streams.add(x);
  ASSERT_EQ(x.get()->channel_info().channel_name,
            streams.back()->channel_info().channel_name);
}

TEST(StreamsTest,
     back_returns_nullptr_when_streams_is_empty_after_initialisation) {
  Streams streams;
  ASSERT_EQ(streams.back(), nullptr);
}

TEST(StreamsTest,
     back_throws_range_error_when_streams_is_empty_after_streams_cleared) {
  Streams streams;
  streams.add(createStream("hello", "world"));
  streams.add(createStream("world", "hello"));
  streams.clearStreams();
  ASSERT_EQ(streams.back(), nullptr);
}

TEST(StreamsTest, check_stream_status_on_streams_after_clear_does_nothing) {
  Streams streams;
  streams.add(createStream("hello", "world"));
  streams.add(createStream("world", "hello"));
  streams.clearStreams();
  streams.checkStreamStatus();
  ASSERT_EQ(nullptr, streams.back().get());
}

TEST(StreamsTest,
     check_stream_status_on_one_stream_with_negative_status_removes_stream) {
  Streams streams;
  auto s = createStream("hello", "world");
  s.get()->error_in_epics(); // sets status to -1
  streams.add(s);
  streams.checkStreamStatus();
  ASSERT_EQ(nullptr, streams.back().get());
}

TEST(
    StreamsTest,
    check_stream_status_with_multiple_streams_with_negative_statuses_removes_all_streams) {
  Streams streams;

  auto s = createStream("hello", "world");
  s.get()->error_in_epics(); // sets status to -1
  auto s2 = createStream("world", "world");
  s2.get()->error_in_epics(); // sets status to -1
  streams.add(s);
  streams.add(s2);
  streams.checkStreamStatus();
  ASSERT_EQ(nullptr, streams.back().get());
  ASSERT_EQ(streams.size(), 0u);
}

TEST(
    StreamsTest,
    check_stream_status_with_multiple_streams_removes_streams_with_negative_statuses_only) {
  Streams streams;

  auto s = createStream("hello", "world");
  s.get()->error_in_epics(); // sets status to -1
  auto s2 = createStream("world", "world");
  streams.add(s);
  streams.add(s2);
  streams.checkStreamStatus();
  ASSERT_EQ(s2.get(), streams.back().get());
  ASSERT_EQ(streams.size(), 1u);
}

TEST(StreamsTest,
     channel_stop_removes_single_channel_with_matched_channel_name) {
  Streams streams;
  auto s = createStream("hello", "world");
  streams.add(s);
  streams.stopChannel("world");
  ASSERT_EQ(nullptr, streams.back().get());
  ASSERT_EQ(streams.size(), 0u);
}

TEST(StreamsTest, channel_stop_removes_all_channels_with_matched_channel_name) {
  Streams streams;

  auto s = createStream("hello", "world");
  auto s2 = createStream("world", "world");

  streams.add(s);
  streams.add(s2);
  streams.stopChannel("world");
  ASSERT_EQ(nullptr, streams.back().get());
  ASSERT_EQ(streams.size(), 0u);
}

TEST(StreamsTest,
     channel_stop_removes_no_channels_with_no_matched_channel_name) {
  Streams streams;
  auto s = createStream("hello", "world");
  auto s2 = createStream("world", "world");
  streams.add(s);
  streams.add(s2);
  streams.stopChannel("nothelloworld");
  ASSERT_EQ(s2.get(), streams.back().get());
  ASSERT_EQ(streams.size(), 2u);
}

TEST(StreamsTest, channel_stop_removes_no_channels_with_no_given_channel_name) {
  Streams streams;
  auto s = createStream("hello", "world");
  auto s2 = createStream("world", "world");
  streams.add(s);
  streams.add(s2);
  streams.stopChannel("");
  ASSERT_EQ(s2.get(), streams.back().get());
  ASSERT_EQ(streams.size(), 2u);
}

TEST(StreamsTest, get_stream_by_name_gets_the_channel_when_it_exists) {
  Streams streams;
  std::string name = "stream1";
  auto s = createStream("some_type", name);
  streams.add(s);
  auto answer = streams.getStreamByChannelName(name);
  ASSERT_EQ(answer->channel_info().channel_name, name);
}

TEST(StreamsTest, get_stream_by_name_does_not_return_stream_when_not_found) {
  Streams streams;
  std::string name = "stream1";
  auto s = createStream("some_type", name);
  streams.add(s);
  auto answer = streams.getStreamByChannelName("incorrect_name");
  ASSERT_EQ(answer, nullptr);
}
