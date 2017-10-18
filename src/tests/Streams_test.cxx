#include <gmock/gmock.h>
#include "../Streams.h"

using namespace testing;

TEST(testStreams, testStreamsAreEmptyOnInitialisation) {
  BrightnESS::ForwardEpicsToKafka::Streams::Streams s;
  ASSERT_EQ(s.size(), 0);
}