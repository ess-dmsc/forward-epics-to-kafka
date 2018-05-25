#include "EpicsClientPeriodic.h"
#include <Stream.h>
#include <chrono>
#include <pv/pvaClient.h>

namespace BrightnESS {
namespace ForwardEpicsToKafka {
namespace EpicsClient {

EpicsClientPeriodic::EpicsClientPeriodic(
    ChannelInfo &channelInfo,
    std::shared_ptr<Ring<std::unique_ptr<FlatBufs::EpicsPVUpdate>>> ring)
    : emit_queue(std::move(ring)), ChannelName(channelInfo.channel_name),
      ProviderType(channelInfo.provider_type) {}

///\fn emit
///\param up the epics PV update containing the PV structure
///\brief calls push_enlarge on the ring buffer to push a pv update object
int EpicsClientPeriodic::emit(
    std::unique_ptr<BrightnESS::FlatBufs::EpicsPVUpdate> up) {
  if (!up) {
    CLOG(6, 1, "empty update?");
    // should never happen, ignore
    return 0;
  }
  emit_queue->push_enlarge(up);

  // here we are, saying goodbye to a good buffer
  up.reset();
  return 1;
}

///\fn PollPVCallback
///\brief checks for pv value, constructs the pv update object and emits it to the ring buffer
void EpicsClientPeriodic::PollPVCallback() {
  ::epics::pvaClient::PvaClientPtr pva =
      ::epics::pvaClient::PvaClient::get("pva ca");
  auto pvStructure = pva->channel(ChannelName, ProviderType, 2.0)
                         ->get()
                         ->getData()
                         ->getPVStructure();
  auto up_ =
      std::unique_ptr<FlatBufs::EpicsPVUpdate>(new FlatBufs::EpicsPVUpdate{});
  auto &up = *up_;
  up.channel = ChannelName;
  up.seq_data = 0;
  up.seq_fwd = 0;
  up.epics_pvstr = pvStructure;
  up.ts_epics_monitor = static_cast<uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count());
  emit(std::move(up_));
}
}
}
}
