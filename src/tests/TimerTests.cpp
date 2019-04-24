#include "Timer.h"
#include <gtest/gtest.h>

using namespace Forwarder;

class TimerTest : public ::testing::Test {
protected:
  void SetUp() override {
    CallbackACalled = false;
    CallbackBCalled = false;
  }
  void testCallbackA() { ++CallbackACalled; }
  void testCallbackB() { ++CallbackBCalled; }

  std::atomic_uint CallbackACalled{0};
  std::atomic_uint CallbackBCalled{0};
};

TEST_F(TimerTest,
       test_can_start_and_stop_a_timer_with_no_registered_callbacks) {
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

TEST_F(TimerTest, test_can_register_a_callback) {
  std::shared_ptr<Sleeper> TestSleeper = std::make_shared<FakeSleeper>();
  std::chrono::milliseconds Interval(1);
  Timer TestTimer(Interval, TestSleeper);
  TestTimer.addCallback([&]() { testCallbackA(); });
}

TEST_F(TimerTest, test_registered_callback_is_executed) {
  std::shared_ptr<Sleeper> TestSleeper = std::make_shared<FakeSleeper>();
  std::chrono::milliseconds Interval(1);
  Timer TestTimer(Interval, TestSleeper);
  TestTimer.addCallback([&]() { testCallbackA(); });
  TestTimer.start();
  auto TestFakeSleeper = std::dynamic_pointer_cast<FakeSleeper>(TestSleeper);
  TestTimer.triggerStop();
  TestFakeSleeper->triggerEndOfSleep(); // Fakes 1 Interval passing
  TestTimer.waitForStop();
  uint32_t ExpectedTimesCalled = 1;
  ASSERT_EQ(ExpectedTimesCalled, CallbackACalled);
}

TEST_F(TimerTest, test_multiple_registered_callbacks_are_executed) {
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
  uint32_t ExpectedTimesCalled = 1;
  ASSERT_EQ(ExpectedTimesCalled, CallbackACalled);
  ASSERT_EQ(ExpectedTimesCalled, CallbackBCalled);
}
