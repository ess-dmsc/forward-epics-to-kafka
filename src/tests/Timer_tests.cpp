#include "Timer.h"
#include <gtest/gtest.h>


using namespace Forwarder;

TEST(TimerTests, testCanStartAndStopATimerWithNoRegisteredCallbacks) {
  std::shared_ptr<Sleeper> TestSleeper = std::make_shared<FakeSleeper>();
  std::chrono::milliseconds Interval(1);
  Timer TestTimer(Interval, TestSleeper);
  TestTimer.start();
  TestTimer.stop();
}
