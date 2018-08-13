#include "FwdMonitorRequester.h"
#include "EpicsClientMonitor.h"
#include "EpicsPVUpdate.h"
#include "helper.h"
#include "logger.h"
#include <atomic>
#include <memory>
#include <pv/pvAccess.h>
#include <pv/pvData.h>

namespace Forwarder {
namespace EpicsClient {

FwdMonitorRequester::FwdMonitorRequester(
    EpicsClientInterface *EpicsClientMonitor, const std::string &ChannelName)
    : channel_name(ChannelName), epics_client(EpicsClientMonitor) {
  static std::atomic<uint32_t> __id{0};
  auto id = __id++;
  name = fmt::format("FwdMonitorRequester-{}", id);
  LOG(Sev::Debug, "FwdMonitorRequester {}", name);
}

FwdMonitorRequester::~FwdMonitorRequester() {
  LOG(Sev::Info, "~FwdMonitorRequester");
}

std::string FwdMonitorRequester::getRequesterName() { return name; }

void FwdMonitorRequester::message(std::string const &Message,
                                  ::epics::pvData::MessageType MessageType) {
  UNUSED_ARG(MessageType);
  LOG(Sev::Debug, "FwdMonitorRequester::message: {}:  {}", name, Message);
}

void FwdMonitorRequester::monitorConnect(
    ::epics::pvData::Status const &Status,
    ::epics::pvData::Monitor::shared_pointer const &Monitor,
    ::epics::pvData::StructureConstPtr const &Structure) {
  UNUSED_ARG(Structure);
  if (!Status.isSuccess()) {
    // NOTE
    // Docs does not say anything about whether we are responsible for any
    // handling of the monitor if non-null?
    LOG(Sev::Error, "monitorConnect is != success for {}", name);
    epics_client->errorInEpics();
  } else {
    if (Status.isOK()) {
      LOG(Sev::Debug, "success and OK");
      Monitor->start();
    } else {
      LOG(Sev::Debug, "success with warning");
    }
  }
}

void FwdMonitorRequester::monitorEvent(
    ::epics::pvData::MonitorPtr const &Monitor) {
  std::vector<std::shared_ptr<FlatBufs::EpicsPVUpdate>> Updates;
  while (true) {
    auto ele = Monitor->poll();
    if (!ele) {
      break;
    }

    static_assert(sizeof(uint64_t) == sizeof(std::chrono::nanoseconds::rep),
                  "Types not compatible");
    uint64_t ts = std::chrono::duration_cast<std::chrono::nanoseconds>(
                      std::chrono::system_clock::now().time_since_epoch())
                      .count();

    // Seems like MonitorElement always returns a Structure type ?
    // The inheritance diagram shows that scalars derive from Field, not from
    // Structure.
    // Does that mean that we never get a scalar here directly??

    auto Update = std::make_shared<FlatBufs::EpicsPVUpdate>();
    Update->channel = channel_name;
    Update->epics_pvstr = epics::pvData::PVStructure::shared_pointer(
        new ::epics::pvData::PVStructure(ele->pvStructurePtr->getStructure()));
    Update->epics_pvstr->copyUnchecked(*ele->pvStructurePtr);
    Monitor->release(ele);
    Update->ts_epics_monitor = ts;
    Updates.push_back(Update);
  }
  for (auto &up : Updates) {
    auto x = epics_client->emit(up);
    if (x != 0) {
      LOG(Sev::Notice, "error can not push update");
    }
  }
}

void FwdMonitorRequester::unlisten(epics::pvData::MonitorPtr const &Monitor) {
  UNUSED_ARG(Monitor);
  LOG(Sev::Debug, "FwdMonitorRequester::unlisten  {}", name);
}
} // namespace EpicsClient
} // namespace Forwarder