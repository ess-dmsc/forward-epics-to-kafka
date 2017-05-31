
#include <iostream>
#include "schemas/fsdc/FSD_Converter.h"
#include "schemas/fsdc/fsdc_FastSamplingData_generated.h"
#include "schemas/fsdc/ifcdaq_data_generated.h"
#include <vector>
#include <pv/ntscalarArray.h>
#include <pv/ntndarray.h>

namespace pvNT = epics::nt;
namespace pv = epics::pvData;

std::uint64_t ExtractTimeStamp(pv::PVStructure::shared_pointer timeStampStructure) {
    if (timeStampStructure.get() == nullptr) {
        //@todo Add error message
        return 0;
    }
    pv::PVTimeStamp pvTS;
    pv::TimeStamp timeStampStruct;
    pvTS.attach(timeStampStructure);
    pvTS.get(timeStampStruct);
  auto step1 = (timeStampStruct.getEpicsSecondsPastEpoch() + 631152000);
  std::uint64_t tempValue = step1 * 1000000000L + timeStampStruct.getNanoseconds();
  return tempValue;
}

template <typename pvScalarArrType, typename fbCreatorFunc>
flatbuffers::Offset<void> MoveDataFromPVToFB(pv::PVFieldPtr scalarArray, flatbuffers::FlatBufferBuilder *builder, fbCreatorFunc f) {
    auto pvValues = dynamic_cast<pvScalarArrType*>(scalarArray.get());
    size_t nrOfElements = pvValues->getLength();
    const auto pvArrPtr = pvValues->view().data();
    std::uint8_t *fbArrPtr;
    auto dataOffset = builder->CreateUninitializedVector(nrOfElements, sizeof(*pvArrPtr), &fbArrPtr);
    std::memcpy(fbArrPtr, pvArrPtr, nrOfElements * sizeof(*pvArrPtr));
    return f(*builder, dataOffset).Union();
}

flatbuffers::Offset<void> ExtractValueArray(pvNT::NTScalarArray::shared_pointer scalarArray, flatbuffers::FlatBufferBuilder *builder, FSD::type &fsdType) {
    auto scalarArrPtr = dynamic_cast<pv::PVScalarArray*>(scalarArray->getValue().get());
    auto elementType = scalarArrPtr->getScalarArray()->getElementType();
    auto convertField = scalarArray->getValue();
    if (pv::ScalarType::pvByte == elementType) {
        fsdType = FSD::type_int8;
        return MoveDataFromPVToFB<pv::PVByteArray>(convertField, builder, FSD::Createint8);
    } else if (pv::ScalarType::pvUByte == elementType) {
        fsdType = FSD::type_uint8;
        return MoveDataFromPVToFB<pv::PVUByteArray>(convertField, builder, FSD::Createuint8);
    } else if (pv::ScalarType::pvShort == elementType) {
        fsdType = FSD::type_int16;
        return MoveDataFromPVToFB<pv::PVShortArray>(convertField, builder, FSD::Createint16);
    } else if (pv::ScalarType::pvUShort == elementType) {
        fsdType = FSD::type_uint16;
        return MoveDataFromPVToFB<pv::PVUShortArray>(convertField, builder, FSD::Createuint16);
    } else if (pv::ScalarType::pvInt == elementType) {
        fsdType = FSD::type_int32;
        return MoveDataFromPVToFB<pv::PVIntArray>(convertField, builder, FSD::Createint32);
    }  else if (pv::ScalarType::pvUInt == elementType) {
        fsdType = FSD::type_uint32;
        return MoveDataFromPVToFB<pv::PVUIntArray>(convertField, builder, FSD::Createuint32);
    } else if (pv::ScalarType::pvLong == elementType) {
        fsdType = FSD::type_int64;
        return MoveDataFromPVToFB<pv::PVLongArray>(convertField, builder, FSD::Createint64);
    } else if (pv::ScalarType::pvULong == elementType) {
        fsdType = FSD::type_uint64;
        return MoveDataFromPVToFB<pv::PVULongArray>(convertField, builder, FSD::Createuint64);
    } else if (pv::ScalarType::pvFloat == elementType) {
        fsdType = FSD::type_float32;
        return MoveDataFromPVToFB<pv::PVFloatArray>(convertField, builder, FSD::Createfloat32);
    } else if (pv::ScalarType::pvDouble == elementType) {
        fsdType = FSD::type_float64;
        return MoveDataFromPVToFB<pv::PVDoubleArray>(convertField, builder, FSD::Createfloat64);
    } else {
        std::cout << "Unknown element type" << std::endl;
        //@todo Error message
    }
    
    std::uint8_t *somePtr;
    auto offset = builder->CreateUninitializedVector(10, 4, &somePtr);
    auto array = FSD::Createint32(*builder, offset);
    return array.Union();
}

flatbuffers::Offset<void> ExtractValueArray(pvNT::NTNDArrayPtr ntNDArray, flatbuffers::FlatBufferBuilder *builder, FSD::type &fsdType) {
    auto valueUnion = ntNDArray->getValue();
    auto scalarArray = valueUnion->get();
    std::string valueID = scalarArray->getField()->getID();
    if ("byte[]" == valueID) {
        fsdType = FSD::type_int8;
        return MoveDataFromPVToFB<pv::PVByteArray>(scalarArray, builder, FSD::Createint8);
    } else if ("ubyte[]" == valueID) {
        fsdType = FSD::type_uint8;
        return MoveDataFromPVToFB<pv::PVUByteArray>(scalarArray, builder, FSD::Createuint8);
    } else if ("short[]" == valueID) {
        fsdType = FSD::type_int16;
        return MoveDataFromPVToFB<pv::PVShortArray>(scalarArray, builder, FSD::Createint16);
    } else if ("ushort[]" == valueID) {
        fsdType = FSD::type_uint16;
        return MoveDataFromPVToFB<pv::PVUShortArray>(scalarArray, builder, FSD::Createuint16);
    } else if ("int[]" == valueID) {
        fsdType = FSD::type_int32;
        return MoveDataFromPVToFB<pv::PVIntArray>(scalarArray, builder, FSD::Createint32);
    }  else if ("uint[]" == valueID) {
        fsdType = FSD::type_uint32;
        return MoveDataFromPVToFB<pv::PVUIntArray>(scalarArray, builder, FSD::Createuint32);
    } else if ("long[]" == valueID) {
        fsdType = FSD::type_int64;
        return MoveDataFromPVToFB<pv::PVLongArray>(scalarArray, builder, FSD::Createint64);
    } else if ("ulong[]" == valueID) {
        fsdType = FSD::type_uint64;
        return MoveDataFromPVToFB<pv::PVULongArray>(scalarArray, builder, FSD::Createuint64);
    } else if ("float[]" == valueID) {
        fsdType = FSD::type_float32;
        return MoveDataFromPVToFB<pv::PVFloatArray>(scalarArray, builder, FSD::Createfloat32);
    } else if ("double[]" == valueID) {
        fsdType = FSD::type_float64;
        return MoveDataFromPVToFB<pv::PVDoubleArray>(scalarArray, builder, FSD::Createfloat64);
    }
    else {
        std::cout << "Unknown element type" << std::endl;
        //@todo Error message
    }
    
    std::uint8_t *somePtr;
    auto offset = builder->CreateUninitializedVector(10, 4, &somePtr);
    auto array = FSD::Createint32(*builder, offset);
    return array.Union();
}

FSD_Converter::FSD_Converter() {
}

FB_uptr FSD_Converter::convert(EpicsPVUpdate const & pvData) {
    auto fb = FB_uptr(new FB);
    auto pvUpdateStruct = pvData.epics_pvstr;
    std::string pvStructType = pvUpdateStruct->getField()->getID();
    auto builder = fb->builder.get();
    //builder->Create
    //@todo Handle failures gracfeully
    //@todo Handle different versions of structures gracefully
    if ("epics:nt/NTScalarArray:1.0" == pvStructType) {
        ExtractNTScalarArrayData(builder, pvUpdateStruct);
    } else if ("epics:nt/NTNDArray:1.0" == pvStructType) {
        ExtractNTNDArrayData(builder, pvUpdateStruct);
    } else if ("ess:fsd/ifcdaq:1.0" == pvStructType) {
      ExtractIfcdaqData(builder, pvUpdateStruct, pvData.channel);
    }else {
        std::cout << "Unknown pv struct type!" << std::endl;
        //@todo Handle unknown structs
    }
    return fb;
}

bool ExtractNTScalarArrayData(flatbuffers::FlatBufferBuilder *builder, pv::PVStructure::shared_pointer pvData) {
    pvNT::NTScalarArrayPtr ntScalarData = pvNT::NTScalarArray::wrap(pvData);
    
    FSD::type dataType;
    auto fbDataOffset = ExtractValueArray(ntScalarData, builder, dataType);
    
    auto tempPVScalarArray = dynamic_cast<const pv::PVScalarArray*>(ntScalarData->getValue().get());
    
    auto nrOfElements = tempPVScalarArray->getLength();
    
    std::vector<std::uint64_t> dimsVector = {nrOfElements,};
    auto dimsOffset = builder->CreateVector(dimsVector);
    
    FSD::FastSamplingDataBuilder fsd_builder(*builder);
    fsd_builder.add_timestamp(ExtractTimeStamp(ntScalarData->getTimeStamp()));
    fsd_builder.add_data(fbDataOffset);
    fsd_builder.add_data_type(dataType);
    fsd_builder.add_dimensions(dimsOffset);
    
    auto fsd_offset = fsd_builder.Finish();
    builder->Finish(fsd_offset);
    return true;
}

bool ExtractNTNDArrayData(flatbuffers::FlatBufferBuilder *builder, epics::pvData::PVStructure::shared_pointer pvData) {
    pvNT::NTNDArrayPtr ntNDArrayData = pvNT::NTNDArray::wrap(pvData);
    
    FSD::type dataType;
    auto fbDataOffset = ExtractValueArray(ntNDArrayData, builder, dataType);
    
    std::vector<std::uint64_t> dimsVector;
    auto ntNDArrayDimsVector = ntNDArrayData->getDimension()->view();
    for (int k = 0; k < ntNDArrayDimsVector.size(); k++) {
        std::uint64_t size = ntNDArrayDimsVector[k]->getSubField<pv::PVScalarValue<std::int32_t>>("size")->get();
        dimsVector.push_back(size);
    }
    auto dimsOffset = builder->CreateVector(dimsVector);
    
    FSD::FastSamplingDataBuilder fsd_builder(*builder);
    fsd_builder.add_timestamp(ExtractTimeStamp(ntNDArrayData->getDataTimeStamp()));
    fsd_builder.add_dimensions(dimsOffset);
    fsd_builder.add_data(fbDataOffset);
    fsd_builder.add_data_type(dataType);
    
    auto fsd_offset = fsd_builder.Finish();
    builder->Finish(fsd_offset);
    return true;
}

bool ExtractIfcdaqData(flatbuffers::FlatBufferBuilder *builder, epics::pvData::PVStructure::shared_pointer pvData, const std::string &pvName) {
  pv::PVDoubleArrayPtr valueArr = pvData->getSubField<pv::PVDoubleArray>("value");
  std::uint8_t *fbValuePtr;
  auto valueOffset = builder->CreateUninitializedVector(valueArr->getLength(), sizeof(double), &fbValuePtr);
  const auto pvValuePtr = valueArr->view().data();
  std::memcpy(fbValuePtr, pvValuePtr, valueArr->getLength() * sizeof(double));
  
  pv::PVDoubleArrayPtr timeArr = pvData->getSubField<pv::PVDoubleArray>("time");
  std::uint8_t *fbTimePtr;
  auto timeOffset = builder->CreateUninitializedVector(timeArr->getLength(), sizeof(double), &fbTimePtr);
  const auto pvTimePtr = timeArr->view().data();
  std::memcpy(fbTimePtr, pvTimePtr, timeArr->getLength() * sizeof(double));
  
  pv::PVTimeStamp timeStamp;
  timeStamp.attach(pvData->getSubField<pv::PVField>("timeStamp"));
  
  auto pvNameOffset = builder->CreateString(pvName);
  
  FSD::ifcdaq_dataBuilder fsd_builder(*builder);
  auto tempTimeStamp = ExtractTimeStamp(pvData->getSubField<pv::PVStructure>("timeStamp"));
  fsd_builder.add_timestamp(tempTimeStamp);
  fsd_builder.add_value(valueOffset);
  fsd_builder.add_time(timeOffset);
  fsd_builder.add_pv(pvNameOffset);
  fsd_builder.add_uniqueId(pvData->getSubField<pv::PVInt>("uniqueId")->get());
  
  auto fsd_offset = fsd_builder.Finish();
  builder->Finish(fsd_offset);
  if (valueArr->getLength() != timeArr->getLength() or valueArr->getLength() == 0) {
    return false;
  }
  return true;
}

MakeFlatBufferFromPVStructure::ptr Info::create_converter() {
    return MakeFlatBufferFromPVStructure::ptr(new FSD_Converter);
}


SchemaRegistry::Registrar<Info> g_registrar_info("fsdc", Info::ptr(new Info));
