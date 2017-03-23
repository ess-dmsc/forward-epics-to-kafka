#pragma once
#include "epics-to-fb.h"
#include "Ring.h"
#include "Stream.h"
#include <thread>
#include <atomic>
#include <mutex>

namespace BrightnESS {
namespace ForwardEpicsToKafka {

class Main;
class ConversionScheduler;
class ConversionPath;
class Stream;

struct ConversionWorkPacket {
~ConversionWorkPacket();
std::unique_ptr<FlatBufs::EpicsPVUpdate> up;
ConversionPath * cp = nullptr;
Stream * stream = nullptr;
};

class ConversionWorker {
public:
ConversionWorker(ConversionScheduler * scheduler, uint32_t queue_size);
int start();
int stop();
int run();
private:
Ring<std::unique_ptr<ConversionWorkPacket>> queue;
std::atomic<uint32_t> do_run {0};
static std::atomic<uint32_t> s_id;
uint32_t id;
std::thread thr;
ConversionScheduler * scheduler = nullptr;
};


// This will get heavily updated soon..
class ConversionScheduler {
public:
ConversionScheduler(Main * main);
int fill(Ring<std::unique_ptr<ConversionWorkPacket>> & queue, uint32_t nfm, uint32_t wid);
private:
Main * main = nullptr;
size_t sid = 0;
std::mutex mx;
};

}
}
