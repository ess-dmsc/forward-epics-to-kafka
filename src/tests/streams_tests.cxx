#include <gmock/gmock.h>
#include "../Streams.h"

using namespace testing;

TEST(StreamsTest, testStreamsAreEmptyOnInitialisation) {
  BrightnESS::ForwardEpicsToKafka::Streams::Streams s;
  ASSERT_EQ(s.size(), 0);
}