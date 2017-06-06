#include "epics-to-fb.h"
#include "epics-pvstr.h"
#include <pv/pvData.h>
#include <pv/pvAccess.h>

namespace BrightnESS {
namespace FlatBufs {

EpicsPVUpdate::EpicsPVUpdate() {}

EpicsPVUpdate::~EpicsPVUpdate() {}

EpicsPVUpdate::EpicsPVUpdate(EpicsPVUpdate const &x)
    : epics_pvstr(x.epics_pvstr), channel(x.channel), seq_data(x.seq_data),
      seq_fwd(x.seq_fwd), ts_epics_monitor(x.ts_epics_monitor), fwdix(x.fwdix),
      teamid(x.teamid) {}
}
}
