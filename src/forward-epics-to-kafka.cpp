#include <atomic>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "Main.h"
#include "MainOpt.h"
#include "logger.h"
#include <fmt/format.h>

namespace BrightnESS {
namespace ForwardEpicsToKafka {}
}

static void handleSignal(int Signal);

class SignalHandler;

static std::atomic<SignalHandler *> g__SignalHandler;

class SignalHandler {
public:
  SignalHandler(std::shared_ptr<BrightnESS::ForwardEpicsToKafka::Main> MainPtr_)
      : MainPtr(MainPtr_) {
    g__SignalHandler.store(this);
    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);
  }
  ~SignalHandler() {
    std::signal(SIGINT, SIG_DFL);
    std::signal(SIGTERM, SIG_DFL);
    g__SignalHandler.store(nullptr);
  }
  void handle(int Signal) { MainPtr->stopForwardingDueToSignal(); }

private:
  std::shared_ptr<BrightnESS::ForwardEpicsToKafka::Main> MainPtr;
};

static void handleSignal(int Signal) {
  if (auto Handler = g__SignalHandler.load()) {
    Handler->handle(Signal);
  }
}

int main(int argc, char **argv) {
  auto op = BrightnESS::ForwardEpicsToKafka::parse_opt(argc, argv);
  auto &opt = *op.second;

  if (!opt.LogFilename.empty()) {
    use_log_file(opt.LogFilename);
  }

  opt.init_logger();

  if (op.first != 0) {
    return 1;
  }

  auto Main = std::make_shared<BrightnESS::ForwardEpicsToKafka::Main>(opt);
  try {
    SignalHandler SignalHandlerInstance(Main);
    Main->forward_epics_to_kafka();
  } catch (std::runtime_error &e) {
    LOG(0, "CATCH runtime error in main watchdog thread: {}", e.what());
    return 1;
  } catch (std::exception &e) {
    LOG(0, "CATCH EXCEPTION in main watchdog thread");
    return 1;
  }
  return 0;
}
