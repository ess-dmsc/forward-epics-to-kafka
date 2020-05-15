// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

#include "CAPathSetup.h"
#include "ConfigParser.h"
#include "Forwarder.h"
#include "MainOpt.h"
#include "Streams.h"
#include "logger.h"
#include <atomic>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <string>

namespace Forwarder {}

static void handleSignal(int Signal);

class SignalHandler;

static std::atomic<SignalHandler *> g__SignalHandler;

class SignalHandler {
public:
  explicit SignalHandler(std::shared_ptr<Forwarder::Forwarder> MainPtr_)
      : MainPtr(std::move(MainPtr_)) {
    g__SignalHandler.store(this);
    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);
  }
  ~SignalHandler() {
    std::signal(SIGINT, SIG_DFL);
    std::signal(SIGTERM, SIG_DFL);
    g__SignalHandler.store(nullptr);
  }
  void handle(int Signal) {
    UNUSED_ARG(Signal);
    MainPtr->stopForwardingDueToSignal();
  }

private:
  std::shared_ptr<Forwarder::Forwarder> MainPtr;
};

static void handleSignal(int Signal) {
  if (auto Handler = g__SignalHandler.load()) {
    Handler->handle(Signal);
  }
}

int main(int argc, char **argv) {
  auto op = Forwarder::parse_opt(argc, argv);
  auto &opt = *op.second;

  try {
    setPathToCaRepeater(argv[0]);
  } catch (std::runtime_error &E) {
    LOG_ERROR("Unable to setup path to caRepeater. The error was: {}",
              E.what());
    LOG_ERROR("Execution of the forward-epics-to-kafka application will "
              "continue but channel access support will be disabled.");
  }

  if (op.first == Forwarder::ParseOptRet::VersionRequested) {
    return 0;
  } else if (op.first == Forwarder::ParseOptRet::Error) {
    return 1;
  }

  auto Main = std::make_shared<Forwarder::Forwarder>(opt);

  try {
    SignalHandler SignalHandlerInstance(Main);
    Main->forward_epics_to_kafka();
  } catch (std::runtime_error &e) {
    getLogger()->critical("CATCH runtime error in main watchdog thread: {}",
                          e.what());
    getLogger()->flush();
    return 1;
  } catch (std::exception &e) {
    getLogger()->critical("CATCH EXCEPTION in main watchdog thread: {}",
                          e.what());
    getLogger()->flush();
    return 1;
  }
  getLogger()->flush();
  return 0;
}
