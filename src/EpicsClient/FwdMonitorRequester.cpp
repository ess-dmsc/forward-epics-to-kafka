#include "FwdMonitorRequester.h"
#include "EpicsClientMonitor.h"
#include "EpicsPVUpdate.h"
#include "logger.h"
#include <atomic>
#include <memory>
#include <pv/pvAccess.h>
#include <pv/pvData.h>

namespace BrightnESS {
namespace ForwardEpicsToKafka {
namespace EpicsClient {

FwdMonitorRequester::FwdMonitorRequester(EpicsClientMonitor *epicsClientMonitor,
                                         const std::string &channel_name)
    : channel_name(channel_name), epics_client(epicsClientMonitor) {
  static std::atomic<uint32_t> __id{0};
  auto id = __id++;
  name = fmt::format("FwdMonitorRequester-{}", id);
  CLOG(7, 6, "FwdMonitorRequester {}", name);
}

FwdMonitorRequester::~FwdMonitorRequester() {
  CLOG(6, 6, "~FwdMonitorRequester");
  CLOG(6, 6, "~FwdMonitorRequester  seq_data_received: {}",
       seq_data_received.to_string());
}

std::string FwdMonitorRequester::getRequesterName() { return name; }

void FwdMonitorRequester::message(std::string const &msg,
                                  ::epics::pvData::MessageType msgT) {
  CLOG(7, 7, "FwdMonitorRequester::message: {}:  {}", name, msg.c_str());
}

void FwdMonitorRequester::monitorConnect(
    ::epics::pvData::Status const &status,
    ::epics::pvData::Monitor::shared_pointer const &monitor,
    ::epics::pvData::StructureConstPtr const &structure) {
  if (!status.isSuccess()) {
    // NOTE
    // Docs does not say anything about whether we are responsible for any
    // handling of the monitor if non-null?
    CLOG(3, 2, "monitorConnect is != success for {}", name);
    epics_client->error_in_epics();
  } else {
    if (status.isOK()) {
      CLOG(7, 7, "success and OK");
      monitor->start();
    } else {
      CLOG(7, 6, "success with warning");
    }
  }
}

void FwdMonitorRequester::monitorEvent(
    ::epics::pvData::MonitorPtr const &monitor) {
  std::vector<std::shared_ptr<FlatBufs::EpicsPVUpdate>> ups;
  while (true) {
    auto ele = monitor->poll();
    if (!ele) {
      break;
    }

    uint64_t seq_data = 0;
    static_assert(sizeof(uint64_t) == sizeof(std::chrono::nanoseconds::rep),
                  "Types not compatible");
    uint64_t ts = std::chrono::duration_cast<std::chrono::nanoseconds>(
                      std::chrono::system_clock::now().time_since_epoch())
                      .count();

    // Seems like MonitorElement always returns a Structure type ?
    // The inheritance diagram shows that scalars derive from Field, not from
    // Structure.
    // Does that mean that we never get a scalar here directly??

    auto up = std::make_shared<FlatBufs::EpicsPVUpdate>();
    up->channel = channel_name;
    up->epics_pvstr = epics::pvData::PVStructure::shared_pointer(
        new ::epics::pvData::PVStructure(ele->pvStructurePtr->getStructure()));
    up->epics_pvstr->copyUnchecked(*ele->pvStructurePtr);
    monitor->release(ele);
    up->seq_fwd = seq;
    up->seq_data = seq_data;
    up->ts_epics_monitor = ts;
    ups.push_back(up);
    seq += 1;
  }
  for (auto &up : ups) {
    up->epics_pvstr->setImmutable();
    auto seq_data = up->seq_data;
    auto x = epics_client->emit(up);
    if (x != 0) {
      LOG(5, "error can not push update {}", seq_data);
    }
  }
}

void FwdMonitorRequester::unlisten(epics::pvData::MonitorPtr const &monitor) {
  CLOG(7, 1, "FwdMonitorRequester::unlisten  {}", name);
}
}
}
}