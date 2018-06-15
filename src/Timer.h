#pragma once
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

namespace Forwarder {

///\class Sleeper
///\brief Interface for wrapper of this_thread::sleep_for functionality
class Sleeper {
public:
  virtual void sleepFor(std::chrono::milliseconds Duration) = 0;
};

///\class RealSleeper
///\brief Wraps this_thread::sleep_for
class RealSleeper : public Sleeper {
public:
  void sleepFor(std::chrono::milliseconds Duration) override {
    std::this_thread::sleep_for(Duration);
  }
};

///\class FakeSleeper
///\brief Instead of using this_thread::sleep_for, blocks until a method is
/// called, this allows for reliable testing of the Timer class
class FakeSleeper : public Sleeper {
public:
  void sleepFor(std::chrono::milliseconds Duration) override {
    std::unique_lock<std::mutex> Lock(Mutex);
    ConditionVariable.wait(Lock, [this] { return Trigger; });
    Trigger = false;
  }

  ///\fn triggerEndOfSleep()
  ///\brief Causes sleepFor() to finish blocking
  void triggerEndOfSleep() {
    {
      std::lock_guard<std::mutex> Lock(Mutex);
      Trigger = true;
    }
    ConditionVariable.notify_one();
  }

private:
  std::condition_variable ConditionVariable;
  std::mutex Mutex;
  bool Trigger = false;
};

using CallbackFunction = std::function<void()>;

///\class Timer
///\brief Timer for the periodic updates.
/// Calls the callback for pushing cached pv values
class Timer {
public:
  explicit Timer(std::chrono::milliseconds Interval,
                 std::shared_ptr<Sleeper> Sleeper)
      : Running(false), IntervalMS(Interval), CallbacksMutex(),
        Sleeper_(Sleeper){};

  ///\fn executionLoop
  ///\brief Loops with the specified period, calling all registered callbacks
  /// with each iteration
  static void executionLoop(Timer *ThisTimer) {
    while (ThisTimer->Running) {
      ThisTimer->Sleeper_->sleepFor(ThisTimer->IntervalMS);
      {
        std::lock_guard<std::mutex> lock(ThisTimer->CallbacksMutex);
        for (const CallbackFunction &Callback : ThisTimer->Callbacks) {
          Callback();
        }
      }
    }
  }

  ///\fn start
  ///\brief starts the timer thread with a call to the callbacks
  void start();

  ///\fn triggerStop
  ///\brief asks the timer thread to stop
  void triggerStop();

  ///\fn waitForStop
  ///\brief blocks until the timer thread has stopped
  void waitForStop();

  ///\fn addCallback
  ///\brief adds a callback to the vector of callbacks for the timer loop to
  /// call
  ///\param Callback the Callback function to add to the Callbacks vector
  void addCallback(CallbackFunction Callback);

private:
  std::atomic_bool Running;
  std::chrono::milliseconds IntervalMS;
  std::mutex CallbacksMutex;
  std::vector<CallbackFunction> Callbacks{};
  std::thread TimerThread;
  std::shared_ptr<Sleeper> Sleeper_;
};
}
