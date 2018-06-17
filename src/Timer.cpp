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

void Timer::start() {
  Running = true;
  ExecutionThread = std::thread(executionLoop, this);
  TimerThread = std::thread(timerLoop, this);
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
