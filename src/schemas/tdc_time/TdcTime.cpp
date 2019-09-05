// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

#include "TdcTime.h"
#include "../../EpicsPVUpdate.h"
#include "../../helper.h"
#include "../../logger.h"
#include <pv/nt.h>
#include <pv/pvIntrospect.h>
#include <tdct_timestamps_generated.h>

namespace TdcTime {

/// \brief Create a tdct flatbuffer from a vector of timestamps.
///
/// \param Name Source name of the data.
/// \param Timestamps The chopper TDC timestamps to be serialized into a
/// flatbuffer.
/// \return A tdct flatbuffer.
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

namespace pvNT = epics::nt;
namespace pv = epics::pvData;

template <typename pvScalarArrType>
std::vector<std::uint64_t> CalcTimestamps(pv::PVFieldPtr const &scalarArray) {
  auto pvValues = dynamic_cast<pvScalarArrType *>(scalarArray.get());
  size_t NrOfElements = pvValues->getLength();
  if (NrOfElements % 2 != 0) {
    throw std::runtime_error(
        "Size of pvAccess array is not a multiple of two.");
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
  const std::string ExpectedStructType{"epics:nt/NTScalarArray:1.0"};
  if (pvStructType != ExpectedStructType) {
    getLogger()->critical(
        "PV is not of expected type for chopper TDC with PV name \"" +
        PvData.channel + "\". Expected \"" + ExpectedStructType + "\" got \"" +
        pvStructType + "\".");
    return {};
  }
  pvNT::NTScalarArrayPtr ntScalarData =
      pvNT::NTScalarArray::wrap(pvUpdateStruct);
  auto scalarArrPtr =
      dynamic_cast<pv::PVScalarArray *>(ntScalarData->getValue().get());
  auto ElementType = scalarArrPtr->getScalarArray()->getElementType();
  if (ElementType != pv::ScalarType::pvInt) {
    getLogger()->error("Array elements are not of expected type for chopper "
                       "TDC with PV name \"" +
                       PvData.channel + "\".");
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
      getLogger()->critical(
          "Unable to convert pv-array of PV \"{}\" into timestamps: {}",
          PvData.channel, E.what());
      return {};
    }
  }
  return {};
}

void Converter::config(std::map<std::string, std::string> const &) {}

std::unique_ptr<FlatBufs::FlatBufferCreator> Info::createConverter() {
  return make_unique<TdcTime::Converter>();
}
} // namespace TdcTime
