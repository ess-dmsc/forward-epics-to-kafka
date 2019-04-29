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
       test_can_start_and_stop_a_timer_with_no_registered_callbacksA) {
  std::chrono::milliseconds Interval(1);
  Timer TestTimer(Interval);
  TestTimer.start();
  TestTimer.waitForStop();
}

TEST_F(TimerTest, test_can_register_a_callbackA) {
  std::chrono::milliseconds Interval(1);
  Timer TestTimer(Interval);
  TestTimer.addCallback([&]() { testCallbackA(); });
}

TEST_F(TimerTest, test_registered_callback_is_executed_at_least_once) {
  std::chrono::milliseconds Interval(5);
  Timer TestTimer(Interval);
  TestTimer.addCallback([&]() { testCallbackA(); });
  TestTimer.start();
  std::this_thread::sleep_for(std::chrono::milliseconds(15));
  TestTimer.waitForStop();
  ASSERT_GT(CallbackACalled, 0);
}

TEST_F(TimerTest,
       test_multiple_registered_callbacks_are_executed_at_least_once) {
  std::chrono::milliseconds Interval(5);
  Timer TestTimer(Interval);
  TestTimer.addCallback([&]() { testCallbackA(); });
  TestTimer.addCallback([&]() { testCallbackB(); });
  TestTimer.start();
  std::this_thread::sleep_for(std::chrono::milliseconds(15));
  TestTimer.waitForStop();
  ASSERT_GT(CallbackACalled, 0);
  ASSERT_GT(CallbackBCalled, 0);
}
