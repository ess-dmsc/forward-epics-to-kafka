#pragma once
#include "logger.h"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

namespace Forwarder {

/// Interface for wrapper of this_thread::sleep_for functionality.
class Sleeper {
public:
  virtual void sleepFor(std::chrono::milliseconds Duration) = 0;
  virtual ~Sleeper() = default;
};

/// Wraps this_thread::sleep_for.
class RealSleeper : public Sleeper {
public:
  void sleepFor(std::chrono::milliseconds Duration) override {
    std::this_thread::sleep_for(Duration);
  }
};

/// Instead of using this_thread::sleep_for, blocks until a method is
/// called, this allows for reliable testing of the Timer class.
class FakeSleeper : public Sleeper {
public:
  void sleepFor(std::chrono::milliseconds Duration) override;

  /// Causes sleepFor() to finish blocking.
  void triggerEndOfSleep();

private:
  std::condition_variable ConditionVariable;
  std::mutex Mutex;
  bool Trigger = false;
};

using CallbackFunction = std::function<void()>;

/// Timer for the periodic updates.
///
/// Calls the callback for pushing cached pv values.
class Timer {
public:
  explicit Timer(std::chrono::milliseconds Interval,
                 std::shared_ptr<Sleeper> Sleeper)
      : Running(false), IntervalMS(Interval), SleeperPtr(std::move(Sleeper)),
        DoIteration(false), IterationComplete(true){};

  /// Executes all registered callbacks when notified to do iteration
  void executionLoop();

  /// Triggers executing registered callbacks at the specified interval.
  ///
  /// Logs an error and waits for callback execution to complete if it takes
  /// longer than the requested interval.
  void timerLoop();

  /// Starts the timer thread with a call to the callbacks.
  void start();

  /// Asks the timer thread to stop.
  void triggerStop();

  /// Blocks until the timer thread has stopped.
  void waitForStop();

  /// Adds a callback to the vector of callbacks for the timer loop to
  /// call.
  ///
  /// \param Callback the Callback function to add to the Callbacks vector.
  void addCallback(CallbackFunction Callback);

private:
  void callCallbacks();
  void waitForExecutionTrigger();
  void notifyOfCompletedIteration();
  void triggerCallbackExecution();
  void waitForPreviousIterationToComplete();

  std::atomic_bool Running;
  std::chrono::milliseconds IntervalMS;
  std::mutex CallbacksMutex;
  std::vector<CallbackFunction> Callbacks{};
  std::thread ExecutionThread;
  std::thread TimerThread;
  std::shared_ptr<Sleeper> SleeperPtr;

  /// For triggering execution of registered callbacks.
  std::atomic_bool DoIteration;
  std::condition_variable DoIterationCV;
  std::mutex DoIterationMutex;

  /// For checking execution of callbacks is complete before triggering next
  /// execution.
  std::atomic_bool IterationComplete;
  std::condition_variable IterationCompleteCV;
  std::mutex IterationCompleteMutex;
};
} // namespace Forwarder
