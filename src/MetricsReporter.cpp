// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

#include "MetricsReporter.h"
#include "CURLReporter.h"
#include "Converter.h"
#include "Forwarder.h"

#ifdef _MSC_VER
std::vector<char> getHostname() {
  std::vector<char> Hostname;
  return Hostname;
}
#else
#include <unistd.h>
std::vector<char> getHostname() {
  std::vector<char> Hostname;
  Hostname.resize(256);
  gethostname(Hostname.data(), Hostname.size());
  if (Hostname.back() != 0) {
    // likely an error
    Hostname.back() = 0;
  }
  return Hostname;
}
#endif

namespace Forwarder {

MetricsReporter::MetricsReporter(
    std::chrono::milliseconds Interval, MainOpt &ApplicationMainOptions,
    std::shared_ptr<InstanceSet> &MainLoopKafkaInstanceSet)
    : IO(), Period(Interval), AsioTimer(IO, Period),
      MainOptions(ApplicationMainOptions),
      KafkaInstanceSet(MainLoopKafkaInstanceSet) {
  Logger->trace("Starting the MetricsTimer");
  AsioTimer.async_wait([this](std::error_code const &Error) {
    if (Error != asio::error::operation_aborted) {
      this->reportMetrics();
    }
  });
  MetricsThread = std::thread(&MetricsReporter::run, this);
}

std::unique_lock<std::mutex> MetricsReporter::get_lock_converters() {
  return std::unique_lock<std::mutex>(converters_mutex);
}

void MetricsReporter::reportMetrics() {
  KafkaInstanceSet->logMetrics();
  auto m1 = g__total_msgs_to_kafka.load();
  auto m2 = m1 / 1000;
  m1 = m1 % 1000;
  uint64_t b1 = g__total_bytes_to_kafka.load();
  auto b2 = b1 / 1024;
  b1 %= 1024;
  auto b3 = b2 / 1024;
  b2 %= 1024;

  Logger->info("m: {:4}.{:03}  b: {:3}.{:03}.{:03}", m2, m1, b3, b2, b1);
  if (CURLReporter::HaveCURL && !MainOptions.InfluxURI.empty()) {
    std::vector<char> Hostname = getHostname();
    int i1 = 0;
    fmt::memory_buffer StatsBuffer;
    for (auto &s : KafkaInstanceSet->getStatsForAllProducers()) {
      format_to(StatsBuffer, "forward-epics-to-kafka,hostname={},set={}",
                Hostname.data(), i1);
      format_to(StatsBuffer, " produced={}", s.produced);
      format_to(StatsBuffer, ",produce_fail={}", s.produce_fail);
      format_to(StatsBuffer, ",local_queue_full={}", s.local_queue_full);
      format_to(StatsBuffer, ",produce_cb={}", s.produce_cb);
      format_to(StatsBuffer, ",produce_cb_fail={}", s.produce_cb_fail); //
      format_to(StatsBuffer, ",poll_served={}", s.poll_served);
      format_to(StatsBuffer, ",msg_too_large={}", s.msg_too_large);
      format_to(StatsBuffer, ",produced_bytes={}", double(s.produced_bytes));
      format_to(StatsBuffer, ",outq={}", s.out_queue);
      format_to(StatsBuffer, "\n");
      ++i1;
    }
    {
      auto lock = get_lock_converters();
      Logger->info("N converters: {}", converters.size());
      i1 = 0;
      for (auto &c : converters) {
        auto stats = c.second.lock()->stats();
        format_to(StatsBuffer, "forward-epics-to-kafka,hostname={},set={}",
                  Hostname.data(), i1);
        int i2 = 0;
        for (auto x : stats) {
          if (i2 > 0) {
            format_to(StatsBuffer, ",");
          } else {
            format_to(StatsBuffer, " ");
          }
          fmt::format_to(StatsBuffer, "{}={}", x.first, x.second);
          ++i2;
        }
        format_to(StatsBuffer, "\n");
        ++i1;
      }
    }
    CURLReporter::send(StatsBuffer, MainOptions.InfluxURI);
  }

  AsioTimer.expires_at(AsioTimer.expires_at() + Period);
  AsioTimer.async_wait([this](std::error_code const &Error) {
    if (Error != asio::error::operation_aborted) {
      this->reportMetrics();
    }
  });
}

MetricsReporter::~MetricsReporter() {
  Logger->trace("Stopping MetricsTimer");
  IO.stop();
  MetricsThread.join();
}
} // namespace Forwarder
