#pragma once

#include "ForwarderInfo.h"
#include "Stream.h"
#include <array>
#include <atomic>
#include <memory>
#include <string>
#include <vector>

namespace BrightnESS {
namespace ForwardEpicsToKafka {
namespace EpicsClient {

using std::array;
using std::vector;
using std::string;

class EpicsClient_impl;

class EpicsClient {
public:
  EpicsClient(Stream *stream, std::shared_ptr<ForwarderInfo> finfo,
              string epics_channel_provider_type, string channel_name);
  ~EpicsClient();
  int emit(std::unique_ptr<FlatBufs::EpicsPVUpdate> up);
  int stop();
  void error_in_epics();

private:
  std::string channel_name;
  std::shared_ptr<ForwarderInfo> finfo;
  std::unique_ptr<EpicsClient_impl> impl;
  Stream *stream = nullptr;
};
}
}
}
