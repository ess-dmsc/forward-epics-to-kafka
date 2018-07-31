#include "../Converter.h"
#include "../Forwarder.h"
#include "../Stream.h"
#include "../Streams.h"
#include "../helper.h"
#include "TestUtils.h"
#include <gmock/gmock.h>

using namespace testing;

class FakeConversionPath : public Forwarder::ConversionPath {
public:
  std::string TopicName;
  std::string SchemaName;

  FakeConversionPath(std::string Topic, std::string Schema)
      : Forwarder::ConversionPath(nullptr, nullptr),
        TopicName(std::move(Topic)), SchemaName(std::move(Schema)) {}

  std::string getKafkaTopicName() const override { return TopicName; }
  std::string getSchemaName() const override { return SchemaName; }
};

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

TEST(StreamTest, add_conversion_path_once_is_okay) {
  auto Stream = createStream("provider", "channel1");
  auto Path = ::make_unique<FakeConversionPath>("Topic", "Schema");
  int ErrorCode = Stream->converter_add(std::move(Path));
  ASSERT_EQ(ErrorCode, 0);
}

TEST(StreamTest, add_conversion_path_twice_is_not_okay) {
  auto Stream = createStream("provider", "channel1");
  auto Path = ::make_unique<FakeConversionPath>("Topic", "Schema");
  Stream->converter_add(std::move(Path));
  // Add a second one
  auto DuplicatePath = ::make_unique<FakeConversionPath>("Topic", "Schema");
  int ErrorCode = Stream->converter_add(std::move(DuplicatePath));
  ASSERT_EQ(ErrorCode, 1);
}
