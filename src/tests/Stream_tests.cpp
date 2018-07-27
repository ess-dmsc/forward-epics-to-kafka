#include "../Converter.h"
#include "../Forwarder.h"
#include "../Stream.h"
#include "../Streams.h"
#include "../helper.h"
#include "TestUtils.h"
#include <gmock/gmock.h>

using namespace testing;

TEST(StreamTest, when_status_is_okay_getting_status_returns_okay) {
  auto Stream = createStream("provider", "channel1");
  ASSERT_EQ(Stream->status(), 0);
}

TEST(StreamTest, when_error_set_getting_status_returns_error) {
  auto Stream = createStream("provider", "channel1");
  Stream->error_in_epics();
  ASSERT_EQ(Stream->status(), -1);
}

TEST(StreamTest, requesting_stop_stops_epics_client) {
  auto Stream = createStream("provider", "channel1");
  auto Client =
      std::static_pointer_cast<FakeEpicsClient>(Stream->getEpicsClient());

  Stream->stop();

  ASSERT_TRUE(Client->IsStopped);
}

TEST(StreamTest, returned_channel_info_contains_correct_values) {
  auto Stream = createStream("provider", "channel1");
  auto Info = Stream->channel_info();
  ASSERT_EQ(Info.provider_type, "provider");
  ASSERT_EQ(Info.channel_name, "channel1");
}

TEST(StreamTest, newly_created_stream_empty_queue) {
  auto Stream = createStream("provider", "channel1");
  ASSERT_EQ(Stream->emit_queue_size(), 0);
}
