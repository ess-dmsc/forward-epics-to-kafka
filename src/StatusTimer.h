// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

#pragma once

#include "logger.h"
#include <asio.hpp>

namespace Forwarder {

class StatusTimer {
public:
  explicit StatusTimer() : AsioTimer(nullptr) {
    // this->start();
  }

  /// Blocks until the timer thread has stopped
  void waitForStop();

  void reportStatus();

  ~StatusTimer();

private:
  void start();
  void run() { IO.run(); }
  asio::io_context IO;
  std::chrono::milliseconds Period;
  asio::steady_timer AsioTimer;
  std::atomic_bool Running;
  std::thread StatusThread;
  SharedLogger Logger = getLogger();
};

} // namespace Forwarder