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
#include "blobs.h"
#include "configuration.h"
#include "logger.h"
#include <fmt/format.h>

namespace BrightnESS {
namespace ForwardEpicsToKafka {}
}

static std::mutex g__mutex_main;
static std::atomic<BrightnESS::ForwardEpicsToKafka::Main *> g__main{nullptr};

void signal_handler(int signal) {
  std::lock_guard<std::mutex> lock(g__mutex_main);
  LOG(0, "SIGNAL {}", signal);
  if (auto x = g__main.load()) {
    x->forwarding_exit();
  }
}

int main(int argc, char **argv) {
  std::signal(SIGINT, signal_handler);
  std::signal(SIGTERM, signal_handler);
  auto op = BrightnESS::ForwardEpicsToKafka::parse_opt(argc, argv);
  auto &opt = *op.second;

  if (!opt.LogFilename.empty()) {
    use_log_file(opt.LogFilename);
  }

  opt.init_logger();

  if (op.first != 0) {
    return 1;
  }

  BrightnESS::ForwardEpicsToKafka::Main main(opt);
  {
    std::lock_guard<std::mutex> lock(g__mutex_main);
    g__main = &main;
  }
  try {
    main.forward_epics_to_kafka();
  } catch (std::runtime_error &e) {
    LOG(0, "CATCH runtime error in main watchdog thread: {}", e.what());
  } catch (std::exception &e) {
    LOG(0, "CATCH EXCEPTION in main watchdog thread");
  }
  std::signal(SIGINT, SIG_DFL);
  std::signal(SIGTERM, SIG_DFL);
  {
    std::lock_guard<std::mutex> lock(g__mutex_main);
    g__main = nullptr;
  }
  return 0;
}
