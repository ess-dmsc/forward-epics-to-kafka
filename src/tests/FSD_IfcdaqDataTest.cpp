#include <gtest/gtest.h>
#include "schemas/fsdc/FSD_Converter.h"
#include "schemas/fsdc/ifcdaq_data_generated.h"
#include <flatbuffers/flatbuffers.h>
#include <pv/pvTimeStamp.h>
#include <pv/timeStamp.h>
#include <pv/standardField.h>
#include <ctime>
#include <map>
#include <vector>
#include <cassert>
#include "CommonConversionTestFunctions.h"

namespace pvDT = epics::pvData;

class IfcdaqDataTestSetUp : public ::testing::Test {
public:
  static void SetUpTestCase() {
  };
  
  static void TearDownTestCase() {
  };
  
  virtual void SetUp(){
    fb_builder.Clear();
  };
  
  virtual void TearDown(){
    
  };
  flatbuffers::FlatBufferBuilder fb_builder;
};

pv::PVStructure::shared_pointer CreateTestIfcdaqData(size_t valueElements = 0, size_t timeElements = 0, int uniqueId = 0, std::uint32_t epochTime = 0, std::uint32_t nsec = 0) {
  pvDT::FieldCreatePtr fieldCreate = pvDT::getFieldCreate();
  pvDT::StandardFieldPtr standardField = pvDT::getStandardField();
  pvDT::PVDataCreatePtr pvDataCreate = pvDT::getPVDataCreate();
  
  pvDT::StructureConstPtr topStructure = fieldCreate->createFieldBuilder()->setId("ess:fsd/ifcdaq:1.0")->addArray("value", pvDT::pvDouble)->addArray("time", pvDT::pvDouble)->add("timeStamp", standardField->timeStamp())->add("uniqueId", pvDT::pvInt)->createStructure();
  pvDT::PVStructurePtr pvStructure = pvDataCreate->createPVStructure(topStructure);
  
  pvDT::PVLongPtr timeSec = pvStructure->getSubField<pvDT::PVStructure>("timeStamp")->getSubField<pv::PVLong>("secondsPastEpoch");
  timeSec->put(epochTime);
  pvDT::PVIntPtr timeNSec = pvStructure->getSubField<pvDT::PVStructure>("timeStamp")->getSubField<pv::PVInt>("nanoseconds");
  timeNSec->put(nsec);
  
  pvDT::PVIntPtr sampleId = pvStructure->getSubField<pvDT::PVInt>("uniqueId");
  sampleId->put(uniqueId);
  
  pvDT::shared_vector<double> valueVector;
  valueVector.resize(valueElements);
  for (int i = 0; i < valueElements; i++) {
    valueVector[i] = i;
  }
  pvDT::PVDoubleArrayPtr sampleData = pvStructure->getSubField<pvDT::PVDoubleArray>("value");
  sampleData->putFrom(freeze(valueVector));
  
  pvDT::shared_vector<double> timeVector;
  timeVector.resize(timeElements);
  for (int i = 0; i < timeElements; i++) {
    timeVector[i] = i;
  }
  pvDT::PVDoubleArrayPtr timeData = pvStructure->getSubField<pvDT::PVDoubleArray>("time");
  timeData->putFrom(freeze(timeVector));
  
  return pvStructure;
}


TEST_F(IfcdaqDataTestSetUp, StructureIdTest) {
  auto testData = CreateTestIfcdaqData(50, 50);
  std::string pvStructType = testData->getField()->getID();
  EXPECT_EQ(pvStructType, "ess:fsd/ifcdaq:1.0");
}

TEST_F(IfcdaqDataTestSetUp, NumberOfValuesTest) {
  auto testData = CreateTestIfcdaqData(111);
  ExtractIfcdaqData(&fb_builder, testData, "someName");
  auto fbResult = FSD::Getifcdaq_data(fb_builder.GetBufferPointer());
  EXPECT_EQ(fbResult->value()->size(), 111);
}

TEST_F(IfcdaqDataTestSetUp, ValueValuesTest) {
  int totalElements = 512;
  auto testData = CreateTestIfcdaqData(totalElements);
  ExtractIfcdaqData(&fb_builder, testData, "someName");
  auto fbResult = FSD::Getifcdaq_data(fb_builder.GetBufferPointer());
  
  pvDT::PVDoubleArrayPtr valueData = testData->getSubField<pvDT::PVDoubleArray>("value");
  auto dataVector = valueData->view();
  
  for (int u = 0; u < fbResult->value()->size(); u++) {
    ASSERT_EQ(fbResult->value()->Get(u), dataVector[u]);
  }
}

TEST_F(IfcdaqDataTestSetUp, NumberOfTimesTest) {
  int totalTimes = 789;
  auto testData = CreateTestIfcdaqData(111, totalTimes);
  ExtractIfcdaqData(&fb_builder, testData, "someName");
  auto fbResult = FSD::Getifcdaq_data(fb_builder.GetBufferPointer());
  EXPECT_EQ(fbResult->time()->size(), totalTimes);
}

TEST_F(IfcdaqDataTestSetUp, TimeValuesTest) {
  int totalTimes = 345;
  auto testData = CreateTestIfcdaqData(0, totalTimes);
  ExtractIfcdaqData(&fb_builder, testData, "someName");
  auto fbResult = FSD::Getifcdaq_data(fb_builder.GetBufferPointer());
  
  pvDT::PVDoubleArrayPtr timeData = testData->getSubField<pvDT::PVDoubleArray>("time");
  auto dataVector = timeData->view();
  
  for (int u = 0; u < fbResult->time()->size(); u++) {
    ASSERT_EQ(fbResult->time()->Get(u), dataVector[u]);
  }
}

TEST_F(IfcdaqDataTestSetUp, PvNameTest) {
  auto testData = CreateTestIfcdaqData();
  std::string testName = "someNameOrAnother";
  ExtractIfcdaqData(&fb_builder, testData, testName);
  auto fbResult = FSD::Getifcdaq_data(fb_builder.GetBufferPointer());
  EXPECT_EQ(fbResult->pv()->str(), testName);
}

TEST_F(IfcdaqDataTestSetUp, UniqueIdTest1) {
  auto testData = CreateTestIfcdaqData();
  ExtractIfcdaqData(&fb_builder, testData, "someNa,e");
  auto fbResult = FSD::Getifcdaq_data(fb_builder.GetBufferPointer());
  EXPECT_EQ(fbResult->uniqueId(), 0);
}

TEST_F(IfcdaqDataTestSetUp, UniqueIdTest2) {
  std::int32_t testId = 1234567;
  auto testData = CreateTestIfcdaqData(0, 0, testId);
  ExtractIfcdaqData(&fb_builder, testData, "someName");
  auto fbResult = FSD::Getifcdaq_data(fb_builder.GetBufferPointer());
  EXPECT_EQ(fbResult->uniqueId(), testId);
}

TEST_F(IfcdaqDataTestSetUp, TimeStampTest1) {
  std::int32_t secTime = 12345;
  std::int32_t nsecTime = 67890;
  auto testData = CreateTestIfcdaqData(0, 0, 0, secTime, nsecTime);
  auto timeStampStruct = testData->getSubField<pvDT::PVStructure>("timeStamp");
  pv::PVTimeStamp someTimeStamp;
  someTimeStamp.attach(timeStampStruct);
  ExtractIfcdaqData(&fb_builder, testData, "someName");
  auto fbResult = FSD::Getifcdaq_data(fb_builder.GetBufferPointer());
  EPICS_To_FB_TimeComparison(someTimeStamp, fbResult);
}

TEST_F(IfcdaqDataTestSetUp, TimeStampTest2) {
  std::int32_t secTime = 12345;
  std::int32_t nsecTime = 67890;
  auto testData = CreateTestIfcdaqData(0, 0, 0, secTime, nsecTime);
  ExtractIfcdaqData(&fb_builder, testData, "someName");
  auto fbResult = FSD::Getifcdaq_data(fb_builder.GetBufferPointer());
  EXPECT_NE(fbResult->timestamp(), 0);
}

TEST_F(IfcdaqDataTestSetUp, LengthDiffTest1) {
  auto testData = CreateTestIfcdaqData(5, 5);
  EXPECT_TRUE(ExtractIfcdaqData(&fb_builder, testData, "someName"));
}

TEST_F(IfcdaqDataTestSetUp, LengthDiffTest2) {
  auto testData = CreateTestIfcdaqData(5, 10);
  EXPECT_FALSE(ExtractIfcdaqData(&fb_builder, testData, "someName"));
}
