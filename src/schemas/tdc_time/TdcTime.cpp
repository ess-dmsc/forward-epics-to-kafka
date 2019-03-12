#include "TdcTime.h"
#include "../../EpicsPVUpdate.h"
#include "../../RangeSet.h"
#include "../../SchemaRegistry.h"
#include "../../helper.h"
#include "../../logger.h"
#include <algorithm>
#include <atomic>
#include <mutex>
#include <pv/nt.h>
#include <pv/ntndarray.h>
#include <pv/ntndarrayAttribute.h>
#include <pv/ntutils.h>
#include <pv/pvEnumerated.h>
#include <senv_data_generated.h>
#include <string>

namespace TdcTime {

namespace pvNT = epics::nt;
namespace pv = epics::pvData;

template <typename pvScalarArrType>
std::vector<std::uint64_t> CalcTimestamps(pv::PVFieldPtr scalarArray) {
  auto pvValues = dynamic_cast<pvScalarArrType *>(scalarArray.get());
  size_t NrOfElements = pvValues->getLength();
  if (NrOfElements % 2 != 0) {
    throw std::runtime_error("Size of pvAccess array is not a power of two.");
  }
  const auto pvArrPtr = pvValues->view().data();
  std::vector<std::uint64_t> RetVector;
  for (size_t i = 0; i < NrOfElements; i += 2) {
    RetVector.push_back(pvArrPtr[i] * 1000000000L + pvArrPtr[i + 1]);
  }
  return RetVector;
}

std::unique_ptr<FlatBufs::FlatbufferMessage>
Converter::create(FlatBufs::EpicsPVUpdate const &PvData) {
  auto pvUpdateStruct = PvData.epics_pvstr;
  std::string pvStructType = pvUpdateStruct->getField()->getID();
  if (pvStructType != "epics:nt/NTScalarArray:1.0") {
    LOG(Sev::Critical, "PV is not of expected type for chopper TDC.");
    return {};
  }
  pvNT::NTScalarArrayPtr ntScalarData =
      pvNT::NTScalarArray::wrap(pvUpdateStruct);
  auto scalarArrPtr =
      dynamic_cast<pv::PVScalarArray *>(ntScalarData->getValue().get());
  auto ElementType = scalarArrPtr->getScalarArray()->getElementType();
  if (ElementType != pv::ScalarType::pvInt) {
    LOG(Sev::Error, "Array elements are not of expected type.");
    return {};
  } else {
    auto ConvertField = ntScalarData->getValue();
    try {
      auto NewTimestamps = CalcTimestamps<pv::PVIntArray>(ConvertField);
      if (NewTimestamps.empty()) {
        return {};
      }
      return generateFlatbufferFromData(PvData.channel, NewTimestamps);
    } catch (std::runtime_error &E) {
      LOG(Sev::Critical, "Unable to convert pv-array into timestamps: {}",
          E.what());
      return {};
    }
  }
  return {};
}

std::unique_ptr<FlatBufs::FlatbufferMessage>
generateFlatbufferFromData(std::string const &Name,
                           std::vector<std::uint64_t> Timestamps) {
  auto ReturnMessage = make_unique<FlatBufs::FlatbufferMessage>();
  auto Builder = ReturnMessage->builder.get();
  std::vector<std::uint16_t> ZeroValues(Timestamps.size());
  std::fill(ZeroValues.begin(), ZeroValues.end(), 0);
  auto ElementVector = Builder->CreateVector(ZeroValues);
  auto TimestampVector = Builder->CreateVector(Timestamps);
  auto FBNameString = Builder->CreateString(Name);
  auto SenvData = SampleEnvironmentDataBuilder(*Builder);
  SenvData.add_Name(FBNameString);
  SenvData.add_Values(ElementVector);
  SenvData.add_Timestamps(TimestampVector);
  SenvData.add_Channel(0);
  SenvData.add_TimeDelta(0.0);
  SenvData.add_MessageCounter(0);
  SenvData.add_TimestampLocation(Location::Middle);
  SenvData.add_PacketTimestamp(Timestamps[0]);
  FinishSampleEnvironmentDataBuffer(*Builder, SenvData.Finish());
  return ReturnMessage;
}

void Converter::config(std::map<std::string, std::string> const &) {}

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
