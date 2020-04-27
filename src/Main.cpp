// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

#include "ConfigParser.h"
#include "Forwarder.h"
#include "MainOpt.h"
#include "Streams.h"
#include "logger.h"
#include <atomic>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fmt/format.h>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <fstream>

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

bool fileExists(std::string const &FullPath) {
  std::fstream InFile(FullPath);
  return InFile.good();
}

void addToPath(std::string const &Path) {
  std::string CurrentPATH{std::getenv("PATH")};
  auto NewPATH = Path + ":" + CurrentPATH;
  setenv("PATH", NewPATH.c_str(), 1);
}

void setPathToCaRepeater(std::string ExecPath) {
  size_t const BufferSize{PATH_MAX};
  char Buffer[BufferSize];
  if (ExecPath[0] != '/') {
    auto ReturnBuffer = getcwd(Buffer, BufferSize);
    if (ReturnBuffer == nullptr) {
      std::cout << "Unable to set PATH to caRepeater.\n";
      return;
    }
    std::string WorkingDirectory{ReturnBuffer};
    ExecPath = WorkingDirectory + "/" + ExecPath;
  }
  auto SlashLoc = ExecPath.rfind("/");
  auto ExecDir = ExecPath.substr(0, SlashLoc);
  if (fileExists(ExecDir + "/caRepeater")) {
    addToPath(ExecDir);
    return;
  }
  SlashLoc = ExecDir.rfind("/");
  auto ExecParentDir = ExecDir.substr(0, SlashLoc);
  if (fileExists(ExecParentDir + "/bin/caRepeater")) {
    addToPath(ExecParentDir + "/bin");
    return;
  }
  std::cout << "Unable to set PATH to caRepeater.\n";
}

int main(int argc, char **argv) {
  setPathToCaRepeater(argv[0]);
  auto op = Forwarder::parse_opt(argc, argv);
  auto &opt = *op.second;

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
