#include "Timer.h"
#include <gtest/gtest.h>

using namespace Forwarder;

class TimerTest : public ::testing::Test {
protected:
  void SetUp() override {
    CallbackACalled = false;
    CallbackBCalled = false;
  }
  void testCallbackA() { CallbackACalled = true; }
  void testCallbackB() { CallbackBCalled = true; }

  std::atomic_bool CallbackACalled{false};
  std::atomic_bool CallbackBCalled{false};
};

TEST_F(TimerTest, testCanStartAndStopATimerWithNoRegisteredCallbacks) {
  std::shared_ptr<Sleeper> TestSleeper = std::make_shared<FakeSleeper>();
  std::chrono::milliseconds Interval(1);
  Timer TestTimer(Interval, TestSleeper);
  TestTimer.start();
  auto TestFakeSleeper = std::dynamic_pointer_cast<FakeSleeper>(TestSleeper);
  TestTimer.triggerStop();
  TestFakeSleeper
      ->triggerEndOfSleep(); // Fakes 1 Interval passing to ensure stop is seen
  TestTimer.waitForStop();
}

TEST_F(TimerTest, testCanRegisterACallback) {
  std::shared_ptr<Sleeper> TestSleeper = std::make_shared<FakeSleeper>();
  std::chrono::milliseconds Interval(1);
  Timer TestTimer(Interval, TestSleeper);
  TestTimer.addCallback([&]() { testCallbackA(); });
}

TEST_F(TimerTest, testRegisteredCallbackIsExecuted) {
  std::shared_ptr<Sleeper> TestSleeper = std::make_shared<FakeSleeper>();
  std::chrono::milliseconds Interval(1);
  Timer TestTimer(Interval, TestSleeper);
  TestTimer.addCallback([&]() { testCallbackA(); });
  TestTimer.start();
  auto TestFakeSleeper = std::dynamic_pointer_cast<FakeSleeper>(TestSleeper);
  TestTimer.triggerStop();
  TestFakeSleeper->triggerEndOfSleep(); // Fakes 1 Interval passing
  TestTimer.waitForStop();
  ASSERT_TRUE(CallbackACalled);
}

TEST_F(TimerTest, testMultipleRegisteredCallbacksAreExecuted) {
  std::shared_ptr<Sleeper> TestSleeper = std::make_shared<FakeSleeper>();
  std::chrono::milliseconds Interval(1);
  Timer TestTimer(Interval, TestSleeper);
  TestTimer.addCallback([&]() { testCallbackA(); });
  TestTimer.addCallback([&]() { testCallbackB(); });
  TestTimer.start();
  auto TestFakeSleeper = std::dynamic_pointer_cast<FakeSleeper>(TestSleeper);
  TestTimer.triggerStop();
  TestFakeSleeper->triggerEndOfSleep(); // Fakes 1 Interval passing
  TestTimer.waitForStop();
  ASSERT_TRUE(CallbackACalled);
  ASSERT_TRUE(CallbackBCalled);
}
