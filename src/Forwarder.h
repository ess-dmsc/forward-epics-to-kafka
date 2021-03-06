// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

#pragma once

#include "Config.h"
#include "ConversionWorker.h"
#include "MainOpt.h"
#include "Streams.h"
#include <algorithm>
#include <atomic>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>

namespace Forwarder {

class MappingAddException : public std::runtime_error {
public:
  explicit MappingAddException(std::string const &What)
      : std::runtime_error(What) {}
};

class Converter;
class Stream;
class Timer;

namespace Config {
class Listener;
} // namespace Config

enum class ForwardingStatus : int32_t {
  NORMAL,
  STOPPED,
};

enum class ForwardingRunState : int {
  RUN = 0,
  STOP = 1,
  STOP_DUE_TO_SIGNAL = 2,
};

class Forwarder {
public:
  explicit Forwarder(MainOpt &Opt);
  ~Forwarder();
  void forward_epics_to_kafka();
  void addMapping(StreamSettings const &StreamInfo);
  void stopForwarding();
  void stopForwardingDueToSignal();
  int conversion_workers_clear();
  int converters_clear();
  std::unique_lock<std::mutex> get_lock_streams();
  std::unique_lock<std::mutex> get_lock_converters();
  // Public for unit tests
  Streams streams;

private:
  MainOpt &main_opt;
  void createFakePVUpdateTimerIfRequired();
  void createPVUpdateTimerIfRequired();
  template <typename T>
  std::shared_ptr<Stream> addStream(ChannelInfo &ChannelInfo);
  std::shared_ptr<InstanceSet> KafkaInstanceSet;
  std::unique_ptr<Config::Listener> config_listener;
  std::unique_ptr<Timer> PVUpdateTimer;
  std::unique_ptr<Timer> GenerateFakePVUpdateTimer;
  std::mutex converters_mutex;
  std::map<std::string, std::weak_ptr<Converter>> converters;
  std::mutex streams_mutex;
  URI createTopicURI(ConverterSettings const &ConverterInfo) const;
  std::mutex conversion_workers_mx;
  std::vector<std::unique_ptr<ConversionWorker>> conversion_workers;
  ConversionScheduler conversion_scheduler;
  std::atomic<ForwardingStatus> forwarding_status{ForwardingStatus::NORMAL};
  std::shared_ptr<KafkaW::Producer> status_producer;
  std::atomic<ForwardingRunState> ForwardingRunFlag{ForwardingRunState::RUN};
  void raiseForwardingFlag(ForwardingRunState ToBeRaised);
  void pushConverterToStream(ConverterSettings const &ConverterInfo,
                             std::shared_ptr<Stream> &Stream);
  void createConfigListener();
  SharedLogger Logger = getLogger();
};

extern std::atomic<uint64_t> g__total_msgs_to_kafka;
extern std::atomic<uint64_t> g__total_bytes_to_kafka;
} // namespace Forwarder
