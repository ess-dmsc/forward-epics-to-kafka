#pragma once
#include "EpicsPVUpdate.h"
#include <memory>

namespace Forwarder {
namespace EpicsClient {

/// Pure virtual interface for the epics client wrappers (monitor/period
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
