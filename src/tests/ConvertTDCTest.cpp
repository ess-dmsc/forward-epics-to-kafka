#include "../EpicsPVUpdate.h"
#include "../schemas/tdc_time/TdcTime.h"
#include <ctime>
#include <flatbuffers/flatbuffers.h>
#include <gtest/gtest.h>
#include <map>
#include <pv/nt.h>
#include <pv/ntscalarArray.h>
#include <pv/pvTimeStamp.h>
#include <pv/timeStamp.h>
#include <tdct_timestamps_generated.h>
#include <vector>

namespace pvNT = epics::nt;
namespace pv = epics::pvData;

class ConvertTDCTest : public ::testing::Test {
public:
  virtual void SetUp() { TestConverter = TdcTime::Converter(); };
  TdcTime::Converter TestConverter;
};

template <typename scalarArrayType> pv::ScalarType GetElementType() {
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

template <typename scalarArrType>
pv::PVStructure::shared_pointer CreateTestNTScalarArray(size_t elements) {
  pvNT::NTScalarArrayBuilderPtr builder = pvNT::NTScalarArray::createBuilder();
  pvNT::NTScalarArrayPtr ntScalarArray =
      builder->value(GetElementType<scalarArrType>())->addTimeStamp()->create();

  typename scalarArrType::svector someValues;
  for (size_t i = 0; i < elements; i++) {
    someValues.push_back(i * 2);
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

pv::PVStructure::shared_pointer CreateTestScalarStruct() {
  auto Builder = pvNT::NTScalar::createBuilder();
  auto PVStruct = Builder->value(pv::pvDouble)->addTimeStamp()->create();
  auto ValueField = PVStruct->getValue<pv::PVDouble>();
  ValueField->put(3.14);
  pv::TimeStamp TS;
  TS.getCurrent();
  pv::PVTimeStamp pvTS;
  PVStruct->attachTimeStamp(pvTS);
  return PVStruct->getPVStructure();
}

TEST_F(ConvertTDCTest, TwoElementSuccess) {
  auto TestData = CreateTestNTScalarArray<pv::PVIntArray>(60);
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

TEST_F(ConvertTDCTest, ThreeElementFailure) {
  auto TestData = CreateTestNTScalarArray<pv::PVIntArray>(3);
  FlatBufs::EpicsPVUpdate Update;
  Update.epics_pvstr = TestData;
  Update.channel = "somestring";
  auto Result = TestConverter.create(Update);
  EXPECT_EQ(Result, nullptr);
}

TEST_F(ConvertTDCTest, ZeroElements) {
  auto TestData = CreateTestNTScalarArray<pv::PVIntArray>(0);
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

TEST_F(ConvertTDCTest, WrongStructFailure) {
  auto TestData = CreateTestScalarStruct();
  FlatBufs::EpicsPVUpdate Update;
  Update.epics_pvstr = TestData;
  Update.channel = "somestring";
  auto Result = TestConverter.create(Update);
  EXPECT_EQ(Result, nullptr);
}

TEST_F(ConvertTDCTest, TestFBContents) {
  auto TestData = CreateTestNTScalarArray<pv::PVIntArray>(4);
  FlatBufs::EpicsPVUpdate Update;
  Update.epics_pvstr = TestData;
  Update.channel = "somestring_alt";
  auto Result = TestConverter.create(Update);
  EXPECT_NE(Result, nullptr);
  auto FBResult = Gettimestamp(Result->builder->GetBufferPointer());
  EXPECT_EQ(FBResult->name()->str(), Update.channel);
  EXPECT_EQ(FBResult->timestamps()->size(), 2);
  EXPECT_EQ(FBResult->timestamps()->Get(0), 2);
  EXPECT_EQ(FBResult->timestamps()->Get(1), 4 * 1000000000L + 6);
}
