#include "../../EpicsPVUpdate.h"
#include "../../RangeSet.h"
#include "../../SchemaRegistry.h"
#include "../../helper.h"
#include "../../logger.h"
#include <f142_logdata_generated.h>
#include "TdcTime.h"
#include <atomic>
#include <mutex>
#include <pv/nt.h>
#include <pv/ntndarray.h>
#include <pv/ntndarrayAttribute.h>
#include <pv/ntutils.h>
#include <pv/pvEnumerated.h>

namespace TdcTime {
  
  namespace pvNT = epics::nt;
  namespace pv = epics::pvData;
  
  std::unique_ptr<FlatBufs::FlatbufferMessage>
  Converter::create(FlatBufs::EpicsPVUpdate const &PvData) {
    auto pvUpdateStruct = PvData.epics_pvstr;
    std::string pvStructType = pvUpdateStruct->getField()->getID();
    if (pvStructType != "epics:nt/NTScalarArray:1.0") {
      LOG(Sev::Critical, "PV is not of expected type for chopper TDC.");
      return {};
    }
    pvNT::NTScalarArrayPtr ntScalarData = pvNT::NTScalarArray::wrap(pvUpdateStruct);
    auto scalarArrPtr = dynamic_cast<pv::PVScalarArray*>(ntScalarData->getValue().get());
    auto ElementType = scalarArrPtr->getScalarArray()->getElementType();
    if (ElementType != pv::ScalarType::pvInt) {
      LOG(Sev::Error, "Array elements are not of expected type.");
      return {};
    }
    return {};
  }
  
  
  void Converter::config(std::map<std::string, std::string> const &) {
    
  }
  
  class Info : public FlatBufs::SchemaInfo {
  public:
    std::unique_ptr<FlatBufs::FlatBufferCreator> createConverter() override;
  };
  
  std::unique_ptr<FlatBufs::FlatBufferCreator> Info::createConverter() {
    return make_unique<Converter>();
  }
  
  FlatBufs::SchemaRegistry::Registrar<Info> g_registrar_info("TdcTime",
                                                             Info::ptr(new Info));
} // namespace TdcTime
