#include "Timer.h"

namespace Forwarder {

void FakeSleeper::sleepFor(std::chrono::milliseconds Duration) {
  std::unique_lock<std::mutex> Lock(Mutex);
  ConditionVariable.wait(Lock, [this] { return Trigger; });
  Trigger = false;
}

void FakeSleeper::triggerEndOfSleep() {
  {
    std::lock_guard<std::mutex> Lock(Mutex);
    Trigger = true;
  }
  ConditionVariable.notify_one();
}

void Timer::executionLoop() {
  while (Running) {
    waitForExecutionTrigger();
    callCallbacks();
    notifyOfCompletedIteration();
  }
};

void Timer::notifyOfCompletedIteration() {
  std::lock_guard<std::mutex> IterationCompleteLock(IterationCompleteMutex);
  IterationComplete = true;
  IterationCompleteCV.notify_one();
}

void Timer::waitForExecutionTrigger() {
  std::unique_lock<std::mutex> DoIterationLock(DoIterationMutex);
  DoIterationCV.wait(DoIterationLock, [this] { return DoIteration == true; });
  DoIteration = false;
}

void Timer::callCallbacks() {
  std::lock_guard<std::mutex> CallbackLock(CallbacksMutex);
  for (const CallbackFunction &Callback : Callbacks) {
    Callback();
  }
}

void Timer::timerLoop() {
  while (Running) {
    Sleeper_->sleepFor(IntervalMS);
    waitForPreviousIterationToComplete();
    triggerCallbackExecution();
  }
};

void Timer::waitForPreviousIterationToComplete() {
  if (!IterationComplete) {
    LOG(3, "Timer could not execute callbacks within specified iteration "
           "period");
    std::unique_lock<std::mutex> Lock(IterationCompleteMutex);
    IterationCompleteCV.wait(Lock,
                             [this] { return IterationComplete == true; });
  }
}

void Timer::triggerCallbackExecution() {
  std::lock_guard<std::mutex> LockDoIteration(DoIterationMutex);
  DoIteration = true;
  IterationComplete = false;
  DoIterationCV.notify_one();
}

void Timer::start() {
  Running = true;
  ExecutionThread = std::thread(&Timer::executionLoop, this);
  TimerThread = std::thread(&Timer::timerLoop, this);
  // Prevent the Timer being told to stop before it has got going
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
};

void Timer::triggerStop() {
  Running = false;
  {
    std::lock_guard<std::mutex> DoIterationLock(DoIterationMutex);
    DoIteration = false;
    DoIterationCV.notify_one();
  }
  {
    std::lock_guard<std::mutex> IterationCompleteLock(IterationCompleteMutex);
    IterationComplete = true;
    IterationCompleteCV.notify_one();
  }
};

void Timer::waitForStop() {
  ExecutionThread.join();
  TimerThread.join();
}

void Timer::addCallback(CallbackFunction Callback) {
  std::lock_guard<std::mutex> lock(CallbacksMutex);
  Callbacks.push_back(Callback);
}
}
