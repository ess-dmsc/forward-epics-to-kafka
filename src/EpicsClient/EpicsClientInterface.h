#pragma once
#include "epics-to-fb.h"
#include <memory>

namespace BrightnESS {
namespace ForwardEpicsToKafka {
namespace EpicsClient {

///\class EpicsClientInterface
///\brief Pure virtual interface for the epics client wrappers (monitor/period
/// updates)
class EpicsClientInterface {
public:
  virtual ~EpicsClientInterface() = default;
  virtual int emit(std::unique_ptr<FlatBufs::EpicsPVUpdate> up) = 0;
  virtual int stop() = 0;
  virtual void error_in_epics() = 0;
  virtual int status() = 0;
};
}
}
}