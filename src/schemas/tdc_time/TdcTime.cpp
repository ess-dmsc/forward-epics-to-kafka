#include "TdcTime.h"
#include "../../EpicsPVUpdate.h"
#include "../../RangeSet.h"
#include "../../logger.h"
#include <algorithm>
#include <atomic>
#include <mutex>
#include <pv/nt.h>
#include <pv/ntndarray.h>
#include <pv/ntndarrayAttribute.h>
#include <pv/ntutils.h>
#include <pv/pvEnumerated.h>
#include <string>
#include <tdct_timestamps_generated.h>

namespace TdcTime {

namespace pvNT = epics::nt;
namespace pv = epics::pvData;

template <typename pvScalarArrType>
std::vector<std::uint64_t> CalcTimestamps(pv::PVFieldPtr const &scalarArray) {
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
    getLogger()->critical("PV is not of expected type for chopper TDC.");
    return {};
  }
  pvNT::NTScalarArrayPtr ntScalarData =
      pvNT::NTScalarArray::wrap(pvUpdateStruct);
  auto scalarArrPtr =
      dynamic_cast<pv::PVScalarArray *>(ntScalarData->getValue().get());
  auto ElementType = scalarArrPtr->getScalarArray()->getElementType();
  if (ElementType != pv::ScalarType::pvInt) {
    getLogger()->error("Array elements are not of expected type.");
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
      getLogger()->critical("Unable to convert pv-array into timestamps: {}",
                            E.what());
      return {};
    }
  }
  return {};
}

std::unique_ptr<FlatBufs::FlatbufferMessage>
generateFlatbufferFromData(std::string const &Name,
                           std::vector<std::uint64_t> const &Timestamps) {
  static std::uint64_t SequenceNumber{0};
  auto ReturnMessage = make_unique<FlatBufs::FlatbufferMessage>();
  auto Builder = ReturnMessage->builder.get();
  std::vector<std::uint16_t> ZeroValues(Timestamps.size());
  std::fill(ZeroValues.begin(), ZeroValues.end(), 0);
  auto TimestampVector = Builder->CreateVector(Timestamps);
  auto FBNameString = Builder->CreateString(Name);
  auto TdcData = timestampBuilder(*Builder);
  TdcData.add_name(FBNameString);
  TdcData.add_timestamps(TimestampVector);
  TdcData.add_sequence_counter(SequenceNumber++);
  FinishtimestampBuffer(*Builder, TdcData.Finish());
  return ReturnMessage;
}

void Converter::config(std::map<std::string, std::string> const &) {}

std::unique_ptr<FlatBufs::FlatBufferCreator> Info::createConverter() {
  return make_unique<TdcTime::Converter>();
}
} // namespace TdcTime
