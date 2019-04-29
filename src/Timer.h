#pragma once
#include "logger.h"
#include <asio.hpp>
#include <atomic>
#include <chrono>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace Forwarder {

using CallbackFunction = std::function<void()>;

class Timer {
public:
  explicit Timer(std::chrono::milliseconds Interval)
      : IO(), Period(Interval), AsioTimer(IO, Period), Running(false) {}

  /// Executes all registered callbacks
  void executeCallbacks();

  /// Starts the timer thread with a call to the callbacks
  void start();

  /// Blocks until the timer thread has stopped
  void waitForStop();

  /// Adds a callback to the vector of callbacks for the timer loop to
  /// call
  ///
  /// \param Callback the Callback function to add to the Callbacks vector.
  void addCallback(CallbackFunction Callback);

private:
  void run() { IO.run(); }
  asio::io_context IO;
  std::chrono::milliseconds Period;
  asio::steady_timer AsioTimer;
  std::atomic_bool Running;
  std::mutex CallbacksMutex;
  std::vector<CallbackFunction> Callbacks{};
  std::thread TimerThread;
};

} // namespace Forwarder
