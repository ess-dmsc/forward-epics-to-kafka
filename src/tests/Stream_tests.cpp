#include "../Converter.h"
#include "../EpicsClient/EpicsClientRandom.h"
#include "../Forwarder.h"
#include "../Stream.h"
#include "../Streams.h"
#include "../helper.h"
#include "StreamTestUtils.h"
#include <gmock/gmock.h>

using namespace testing;
using namespace Forwarder;
using namespace moodycamel;

/// A fake conversion path for use in testing.
class FakeConversionPath : public ConversionPath {
public:
  std::string TopicName;
  std::string SchemaName;

  FakeConversionPath(std::string Topic, std::string Schema)
      : ConversionPath(nullptr, nullptr), TopicName(std::move(Topic)),
        SchemaName(std::move(Schema)) {}

  std::string getKafkaTopicName() const override { return TopicName; }
  std::string getSchemaName() const override { return SchemaName; }
};

/// Create a stream of random values.
///
/// \param Conversions The number of conversion paths to create
/// \param Entries The number of values to put in the stream
/// \return The stream
std::shared_ptr<Stream> createStreamWithEntries(size_t Conversions,
                                                size_t Entries) {
  auto Stream = createStreamRandom("provider", "channel");

  for (size_t i = 0; i < Conversions; ++i) {
    auto Path = ::make_unique<FakeConversionPath>("Topic" + std::to_string(i),
                                                  "Schema" + std::to_string(i));
    Stream->addConverter(std::move(Path));
  }

  // Add multiple updates
  auto Client = std::static_pointer_cast<EpicsClient::EpicsClientRandom>(
      Stream->getEpicsClient());
  for (size_t i = 0; i < Entries; ++i) {
    Client->generateFakePVUpdate();
  }

  return Stream;
}

/// Clear the queue.
///
/// Unless we clear the queue the manually the ConversionPath won't be
/// destructed and the test will hang.
///
/// Note: this won't work in a teardown as the test will hang before the
/// teardown starts.
///
/// \param queue
void clearQueue(ConcurrentQueue<std::unique_ptr<ConversionWorkPacket>> &queue) {
  std::unique_ptr<ConversionWorkPacket> Data;
  while (queue.try_dequeue(Data)) {
    Data.reset();
  }
}

TEST(StreamTest, when_status_is_okay_getting_status_returns_okay) {
  auto Stream = createStream("provider", "channel1");
  ASSERT_EQ(Stream->status(), 0);
}

TEST(StreamTest, when_error_set_getting_status_returns_error) {
  auto Stream = createStream("provider", "channel1");
  Stream->setEpicsError();
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
  auto Info = Stream->getChannelInfo();
  ASSERT_EQ(Info.provider_type, "provider");
  ASSERT_EQ(Info.channel_name, "channel1");
}

TEST(StreamTest, newly_created_stream_has_empty_queue) {
  auto Stream = createStream("provider", "channel1");
  ASSERT_EQ(Stream->getQueueSize(), 0u);
}

TEST(StreamTest, stream_with_updates_has_non_empty_queue) {
  auto Stream = createStreamWithEntries(1, 2);
  ASSERT_EQ(Stream->getQueueSize(), 2u);
}

TEST(StreamTest, add_conversion_path_once_is_okay) {
  auto Stream = createStream("provider", "channel1");
  auto Path = ::make_unique<FakeConversionPath>("Topic", "Schema");
  int ErrorCode = Stream->addConverter(std::move(Path));
  ASSERT_EQ(ErrorCode, 0);
}

TEST(StreamTest, add_conversion_path_twice_is_not_okay) {
  auto Stream = createStream("provider", "channel1");
  auto Path = ::make_unique<FakeConversionPath>("Topic", "Schema");
  Stream->addConverter(std::move(Path));
  // Add a second one
  auto DuplicatePath = ::make_unique<FakeConversionPath>("Topic", "Schema");
  int ErrorCode = Stream->addConverter(std::move(DuplicatePath));
  ASSERT_EQ(ErrorCode, 1);
}

TEST(StreamTest, filling_queue_from_empty_stream_gives_no_data) {
  auto Stream = createStreamRandom("provider", "channel1");
  ConcurrentQueue<std::unique_ptr<ConversionWorkPacket>> queue;
  auto NumEnqueued = Stream->fillConversionQueue(queue, 10);
  ASSERT_EQ(NumEnqueued, 0u);
}

TEST(StreamTest, filling_queue_from_stream_gives_data) {
  size_t NumEntries = 2;
  auto Stream = createStreamWithEntries(1, NumEntries);
  ConcurrentQueue<std::unique_ptr<ConversionWorkPacket>> queue;
  auto NumEnqueued = Stream->fillConversionQueue(queue, 10);
  ASSERT_EQ(NumEnqueued, NumEntries);

  // Post test clean up.
  clearQueue(queue);
}

TEST(StreamTest, filling_queue_when_multiple_conversion_paths_gives_data) {
  size_t NumEntries = 2;
  size_t NumConversions = 2;
  auto Stream = createStreamWithEntries(NumConversions, NumEntries);
  ConcurrentQueue<std::unique_ptr<ConversionWorkPacket>> queue;
  auto NumEnqueued = Stream->fillConversionQueue(queue, 10);
  ASSERT_EQ(NumEnqueued, NumConversions * NumEntries);

  // Post test clean up.
  clearQueue(queue);
}

TEST(
    StreamTest,
    filling_queue_when_max_buffer_less_than_data_in_stream_gives_correct_amount) {
  auto Stream = createStreamWithEntries(1, 10);
  ConcurrentQueue<std::unique_ptr<ConversionWorkPacket>> queue;

  uint32_t Max = 5;
  auto NumEnqueued = Stream->fillConversionQueue(queue, Max);
  ASSERT_EQ(NumEnqueued, Max);

  // Post test clean up.
  clearQueue(queue);
}

TEST(StreamTest,
     filling_queue_when_max_buffer_less_than_number_conversions_gives_no_data) {
  auto Stream = createStreamWithEntries(10, 10);
  ConcurrentQueue<std::unique_ptr<ConversionWorkPacket>> queue;

  uint32_t Max = 5;
  auto NumEnqueued = Stream->fillConversionQueue(queue, Max);
  ASSERT_EQ(NumEnqueued, 0u);

  // Post test clean up.
  clearQueue(queue);
}

TEST(StreamTest, filling_queue_when_no_conversions_gives_no_data) {
  auto Stream = createStreamWithEntries(0, 5);
  ConcurrentQueue<std::unique_ptr<ConversionWorkPacket>> queue;

  auto NumEnqueued = Stream->fillConversionQueue(queue, 10);
  ASSERT_EQ(NumEnqueued, 0u);

  // Post test clean up.
  clearQueue(queue);
}
