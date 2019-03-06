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
      std::unique_ptr<FlatBufs::FlatbufferMessage>
    Converter::create(FlatBufs::EpicsPVUpdate const &) {
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
