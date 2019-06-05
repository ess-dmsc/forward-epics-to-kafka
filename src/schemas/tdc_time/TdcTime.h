#pragma once

#include "../../FlatBufferCreator.h"
#include "../../SchemaRegistry.h"
#include <map>
#include <string>

namespace TdcTime {
/// \brief Implements a class that is used to forward chopper TDC data in the
/// form of NTScalarArray.
class Converter : public FlatBufs::FlatBufferCreator {
public:
  /// \brief Ingests NTScalarArray data and ouputs tdct flatbuffers. Takes n
  /// elements and returns n/2 UNIX epoch timestamps (in ns).
  /// \param PvData Requries that data is in the form of a NtScalarArray
  /// containing uint32
  /// elements and a size that is a multiple of two.
  /// \return Pointer (std::unique_ptr) to a flatbuffer if PvData contained a
  /// scalar array with
  /// a multiple of two elements (and greater than zero). Returns nullptr
  /// otherwise.
  std::unique_ptr<FlatBufs::FlatbufferMessage>
  create(FlatBufs::EpicsPVUpdate const &PvData) override;

  /// \brief Does not do anything.
  void
  config(std::map<std::string, std::string> const &KafkaConfiguration) override;
};

/// \brief Factory class.
class Info : public FlatBufs::SchemaInfo {
public:
  /// \brief Factory member function.
  std::unique_ptr<FlatBufs::FlatBufferCreator> createConverter() override;
};
} // namespace TdcTime
