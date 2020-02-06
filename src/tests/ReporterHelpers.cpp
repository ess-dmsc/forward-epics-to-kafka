#include "ReporterHelpers.h"
#include <chrono>
#include <thread>

using namespace std::chrono_literals;

void waitForReportIterations(uint32_t const NumberOfIterationsToWaitFor,
                             std::atomic<uint32_t> &ReportIterations) {
  auto PollInterval = 50ms;
  auto TotalWait = 0ms;
  auto TimeOut = 10s; // Max wait time, prevent test hanging indefinitely
  while (ReportIterations < NumberOfIterationsToWaitFor &&
         TotalWait < TimeOut) {
    std::this_thread::sleep_for(PollInterval);
    TotalWait += PollInterval;
  }
}
