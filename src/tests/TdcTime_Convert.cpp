#include <gtest/gtest.h>
#include <flatbuffers/flatbuffers.h>
#include <pv/ntscalarArray.h>
#include <pv/pvTimeStamp.h>
#include <pv/timeStamp.h>
#include <ctime>
#include <map>
#include <vector>
#include "../schemas/tdc_time/TdcTime.h"
#include "../EpicsPVUpdate.h"
//#include <cassert>
//#include "CommonConversionTestFunctions.h"

namespace pvNT = epics::nt;
namespace pv = epics::pvData;

class ConvertTDCTest : public ::testing::Test {
public:
    virtual void SetUp() {
      TestConverter = TdcTime::Converter();
//        fb_builder.Clear();
    };
    
    virtual void TearDown(){
        
    };
  TdcTime::Converter TestConverter;
//    flatbuffers::FlatBufferBuilder fb_builder;
};

//template <typename pv_typeName, typename FSD_typeName>
//void CompareArrays(pvNT::NTScalarArray::shared_pointer scalarArray, FSD::FastSamplingData const *fsd_data) {
//    auto fsdArr = static_cast<const FSD_typeName*>(fsd_data->data())->value();
//    auto pvArr = scalarArray->getValue<pv_typeName>();
//    auto pvArrPtr = pvArr->view().data();
//    for (int i = 0; i < fsdArr->size(); i++) {
//        //std::cout << (*fsdArr)[i] << ":" << pvArrPtr[i] << std::endl;
//        ASSERT_EQ((*fsdArr)[i], pvArrPtr[i]);
//    }
//}

//void CompareElements(pvNT::NTScalarArray::shared_pointer scalarArray, FSD::FastSamplingData const *fsd_data) {
//    if (fsd_data->data_type() == FSD::type_int8) {
//        CompareArrays<pv::PVByteArray, FSD::int8>(scalarArray, fsd_data);
//    } else if (fsd_data->data_type() == FSD::type_uint8) {
//        CompareArrays<pv::PVUByteArray, FSD::uint8>(scalarArray, fsd_data);
//    } else if (fsd_data->data_type() == FSD::type_int16) {
//        CompareArrays<pv::PVShortArray, FSD::int16>(scalarArray, fsd_data);
//    } else if (fsd_data->data_type() == FSD::type_uint16) {
//        CompareArrays<pv::PVUShortArray, FSD::uint16>(scalarArray, fsd_data);
//    } else if (fsd_data->data_type() == FSD::type_int32) {
//        CompareArrays<pv::PVIntArray, FSD::int32>(scalarArray, fsd_data);
//    } else if (fsd_data->data_type() == FSD::type_uint32) {
//        CompareArrays<pv::PVUIntArray, FSD::uint32>(scalarArray, fsd_data);
//    } else if (fsd_data->data_type() == FSD::type_int64) {
//        CompareArrays<pv::PVLongArray, FSD::int64>(scalarArray, fsd_data);
//    } else if (fsd_data->data_type() == FSD::type_uint64) {
//        CompareArrays<pv::PVULongArray, FSD::uint64>(scalarArray, fsd_data);
//    } else if (fsd_data->data_type() == FSD::type_float32) {
//        CompareArrays<pv::PVFloatArray, FSD::float32>(scalarArray, fsd_data);
//    } else if (fsd_data->data_type() == FSD::type_float64) {
//        CompareArrays<pv::PVDoubleArray, FSD::float64>(scalarArray, fsd_data);
//    } else {
//        FAIL();
//    }
//
//}


//void EPICS_To_FB_ValueComparison(pvNT::NTScalarArray::shared_pointer scalarArray, FSD::FastSamplingData const *fsd_data) {
//    std::map<pv::ScalarType, FSD::type> pvToFBTypeMap = {{pv::ScalarType::pvByte, FSD::type_int8},
//    {pv::ScalarType::pvUByte, FSD::type_uint8}, {pv::ScalarType::pvShort, FSD::type_int16},
//        {pv::ScalarType::pvUShort, FSD::type_uint16}, {pv::ScalarType::pvInt, FSD::type_int32},
//        {pv::ScalarType::pvUInt, FSD::type_uint32}, {pv::ScalarType::pvLong, FSD::type_int64},
//        {pv::ScalarType::pvULong, FSD::type_uint64}, {pv::ScalarType::pvFloat, FSD::type_float32},
//        {pv::ScalarType::pvDouble, FSD::type_float64} };
//
//    auto tempPVScalarArray = dynamic_cast<const pv::PVScalarArray*>(scalarArray->getValue().get());
//
//    auto pvElemType = tempPVScalarArray->getScalarArray()->getElementType();
//    auto nrOfElements = tempPVScalarArray->getLength();
//
//
//    ASSERT_NE(pvToFBTypeMap.find(pvElemType), pvToFBTypeMap.end());
//
//    EXPECT_EQ(pvToFBTypeMap[pvElemType], fsd_data->data_type());
//
//    EXPECT_EQ(GetNrOfElements(fsd_data), nrOfElements);
//
//    EXPECT_EQ(GetNrOfElements(fsd_data), nrOfElements);
//
//    CompareElements(scalarArray, fsd_data);
//}

template <typename scalarArrayType>
pv::ScalarType GetElementType() {
    if (std::is_same<scalarArrayType, pv::PVByteArray>::value) {
        return pv::ScalarType::pvByte;
    } else if (std::is_same<scalarArrayType, pv::PVUByteArray>::value) {
        return pv::ScalarType::pvUByte;
    } else if (std::is_same<scalarArrayType, pv::PVShortArray>::value) {
        return pv::ScalarType::pvShort;
    } else if (std::is_same<scalarArrayType, pv::PVUShortArray>::value) {
        return pv::ScalarType::pvUShort;
    } else if (std::is_same<scalarArrayType, pv::PVIntArray>::value) {
        return pv::ScalarType::pvInt;
    } else if (std::is_same<scalarArrayType, pv::PVUIntArray>::value) {
        return pv::ScalarType::pvUInt;
    } else if (std::is_same<scalarArrayType, pv::PVLongArray>::value) {
        return pv::ScalarType::pvLong;
    } else if (std::is_same<scalarArrayType, pv::PVULongArray>::value) {
        return pv::ScalarType::pvULong;
    } else if (std::is_same<scalarArrayType, pv::PVFloatArray>::value) {
        return pv::ScalarType::pvFloat;
    } else if (std::is_same<scalarArrayType, pv::PVDoubleArray>::value) {
        return pv::ScalarType::pvDouble;
    } else {
        assert(false);
    }
    return pv::ScalarType::pvInt;
}

template<typename scalarArrType>
pv::PVStructure::shared_pointer CreateTestNTScalarArray(size_t elements) {
    pvNT::NTScalarArrayBuilderPtr builder = pvNT::NTScalarArray::createBuilder();
    pvNT::NTScalarArrayPtr ntScalarArray = builder->value(GetElementType<scalarArrType>())->addTimeStamp()->create();
    
    typename scalarArrType::svector someValues;
    for (int i = 0; i < elements; i++) {
        someValues.push_back(i*2);
    }
    
    auto pvValueField = ntScalarArray->getValue<scalarArrType>();
    pvValueField->replace(freeze(someValues));
    
    pv::TimeStamp timeStamp;
    timeStamp.fromTime_t(time(nullptr));
    timeStamp += 0.123456789;
    pv::PVTimeStamp pvTimeStamp;
    ntScalarArray->attachTimeStamp(pvTimeStamp);
    pvTimeStamp.set(timeStamp);
    
    return ntScalarArray->getPVStructure();
}


TEST_F(ConvertTDCTest, TwoElementSuccess) {
  auto TestData = CreateTestNTScalarArray<pv::PVIntArray>(2);
  FlatBufs::EpicsPVUpdate Update;
  Update.epics_pvstr = TestData;
  Update.channel = "somestring";
  auto Result = TestConverter.create(Update);
  EXPECT_NE(Result, nullptr);
}

TEST_F(ConvertTDCTest, OneElementFailure) {
  auto TestData = CreateTestNTScalarArray<pv::PVIntArray>(1);
  FlatBufs::EpicsPVUpdate Update;
  Update.epics_pvstr = TestData;
  Update.channel = "somestring";
  auto Result = TestConverter.create(Update);
  EXPECT_EQ(Result, nullptr);
}

TEST_F(ConvertTDCTest, WrongTypeFailure) {
  auto TestData = CreateTestNTScalarArray<pv::PVDoubleArray>(2);
  FlatBufs::EpicsPVUpdate Update;
  Update.epics_pvstr = TestData;
  Update.channel = "somestring";
  auto Result = TestConverter.create(Update);
  EXPECT_EQ(Result, nullptr);
}



//TEST_F(NTScalarArrayTestSetUp, NTScalarArrayToTimeTest) {
//    auto testData = CreateTestNTScalarArray<pv::PVIntArray>(100);
//    ExtractNTScalarArrayData(&fb_builder, testData);
//    auto fbResult = FSD::GetFastSamplingData(fb_builder.GetBufferPointer());
//    auto tempNTScalarArray = pvNT::NTScalarArray::wrap(testData);
//    pv::PVTimeStamp timeStamp;
//    timeStamp.attach(testData->getSubField<pv::PVStructure>("timeStamp"));
//    EPICS_To_FB_TimeComparison(timeStamp, fbResult);
//}
//
//TEST_F(NTScalarArrayTestSetUp, NTScalarArrayToValueTest) {
//    auto testData = CreateTestNTScalarArray<pv::PVIntArray>(100);
//    ExtractNTScalarArrayData(&fb_builder, testData);
//    auto fbResult = FSD::GetFastSamplingData(fb_builder.GetBufferPointer());
//    auto tempNTScalarArray = pvNT::NTScalarArray::wrap(testData);
//    EPICS_To_FB_ValueComparison(tempNTScalarArray, fbResult);
//}
//
//template <typename scalarArrType>
//void TestNTScalarArrConversion() {
//    flatbuffers::FlatBufferBuilder local_fb_builder;
//    auto testData = CreateTestNTScalarArray<scalarArrType>(100);
//    ExtractNTScalarArrayData(&local_fb_builder, testData);
//    auto fbResult = FSD::GetFastSamplingData(local_fb_builder.GetBufferPointer());
//    auto tempNTScalarArray = pvNT::NTScalarArray::wrap(testData);
//    EPICS_To_FB_ValueComparison(tempNTScalarArray, fbResult);
//}
//
//TEST_F(NTScalarArrayTestSetUp, NTScalarArrayAllTypesTest) {
//    TestNTScalarArrConversion<pv::PVByteArray>();
//    TestNTScalarArrConversion<pv::PVUByteArray>();
//
//    TestNTScalarArrConversion<pv::PVShortArray>();
//    TestNTScalarArrConversion<pv::PVUShortArray>();
//
//    TestNTScalarArrConversion<pv::PVIntArray>();
//    TestNTScalarArrConversion<pv::PVUIntArray>();
//
//    TestNTScalarArrConversion<pv::PVLongArray>();
//    TestNTScalarArrConversion<pv::PVULongArray>();
//
//    TestNTScalarArrConversion<pv::PVFloatArray>();
//    TestNTScalarArrConversion<pv::PVDoubleArray>();
//}
//
//TEST_F(NTScalarArrayTestSetUp, TestDimensions) {
//    auto testData = CreateTestNTScalarArray<pv::PVDoubleArray>(111);
//    ExtractNTScalarArrayData(&fb_builder, testData);
//    auto fbResult = FSD::GetFastSamplingData(fb_builder.GetBufferPointer());
//    auto fbValueArr = static_cast<const FSD::float64*>(fbResult->data());
//    std::uint64_t elemFromDims = 1;
//    for (auto iter = fbResult->dimensions()->begin(); iter != fbResult->dimensions()->end(); iter++) {
//        elemFromDims *= *iter;
//    }
//    EXPECT_EQ(elemFromDims, fbValueArr->value()->size());
//    auto tempNTScalarArray = pvNT::NTScalarArray::wrap(testData);
//    auto tempPVScalarArray = dynamic_cast<const pv::PVScalarArray*>(tempNTScalarArray->getValue().get());
//
//    auto PVnrOfElements = tempPVScalarArray->getLength();
//    EXPECT_EQ(elemFromDims, PVnrOfElements);
//}
