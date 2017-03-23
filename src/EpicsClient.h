#pragma once

#include <memory>
#include <atomic>
#include <array>
#include <vector>
#include <string>
#include "Stream.h"
#include "ForwarderInfo.h"

namespace BrightnESS {
namespace ForwardEpicsToKafka {
namespace EpicsClient {

using std::array;
using std::vector;
using std::string;

class EpicsClient_impl;

class EpicsClient {
public:
EpicsClient(Stream * stream, std::shared_ptr<ForwarderInfo> finfo, string channel_name);
~EpicsClient();
int emit(std::unique_ptr<FlatBufs::EpicsPVUpdate> up);
int stop();
private:
std::string channel_name;
std::shared_ptr<ForwarderInfo> finfo;
std::unique_ptr<EpicsClient_impl> impl;
Stream * stream = nullptr;
};

}
}
}
