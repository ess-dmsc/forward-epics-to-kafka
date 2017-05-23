#include <gtest/gtest.h>
#include "schemas/fsdc/FSD_Converter.h"
#include <flatbuffers/flatbuffers.h>
#include "schemas/fsdc/fsdc_FastSamplingData_generated.h"
#include <pv/ntndarray.h>
#include <pv/pvTimeStamp.h>
#include <pv/timeStamp.h>
#include <pv/pvIntrospect.h>
#include <ctime>
#include <map>
#include <vector>
#include <cassert>
#include "CommonConversionTestFunctions.h"
#include <memory>

namespace pvNT = epics::nt;
namespace pv = epics::pvData;

class NTNDArrayTestSetUp : public ::testing::Test {
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

template <typename pv_typeName, typename FSD_typeName>
void CompareArrays(pv::PVScalarArrayPtr scalarArray, FSD::FastSamplingData const *fsd_data) {
    auto fsdArr = static_cast<const FSD_typeName*>(fsd_data->data())->value();
    auto pvArr = dynamic_cast<const pv_typeName*>(scalarArray.get());// scalarArray->getValue<pv_typeName>();
    auto pvArrPtr = pvArr->view().data();
    for (int i = 0; i < fsdArr->size(); i++) {
        ASSERT_EQ((*fsdArr)[i], pvArrPtr[i]);
    }
}

void CompareElements(pv::PVScalarArrayPtr scalarArray, FSD::FastSamplingData const *fsd_data) {
    if (fsd_data->data_type() == FSD::type_int8) {
        CompareArrays<pv::PVByteArray, FSD::int8>(scalarArray, fsd_data);
    } else if (fsd_data->data_type() == FSD::type_uint8) {
        CompareArrays<pv::PVUByteArray, FSD::uint8>(scalarArray, fsd_data);
    } else if (fsd_data->data_type() == FSD::type_int16) {
        CompareArrays<pv::PVShortArray, FSD::int16>(scalarArray, fsd_data);
    } else if (fsd_data->data_type() == FSD::type_uint16) {
        CompareArrays<pv::PVUShortArray, FSD::uint16>(scalarArray, fsd_data);
    } else if (fsd_data->data_type() == FSD::type_int32) {
        CompareArrays<pv::PVIntArray, FSD::int32>(scalarArray, fsd_data);
    } else if (fsd_data->data_type() == FSD::type_uint32) {
        CompareArrays<pv::PVUIntArray, FSD::uint32>(scalarArray, fsd_data);
    } else if (fsd_data->data_type() == FSD::type_int64) {
        CompareArrays<pv::PVLongArray, FSD::int64>(scalarArray, fsd_data);
    } else if (fsd_data->data_type() == FSD::type_uint64) {
        CompareArrays<pv::PVULongArray, FSD::uint64>(scalarArray, fsd_data);
    } else if (fsd_data->data_type() == FSD::type_float32) {
        CompareArrays<pv::PVFloatArray, FSD::float32>(scalarArray, fsd_data);
    } else if (fsd_data->data_type() == FSD::type_float64) {
        CompareArrays<pv::PVDoubleArray, FSD::float64>(scalarArray, fsd_data);
    } else {
        FAIL();
    }
}

void EPICS_To_FB_ValueComparison(pvNT::NTNDArray::shared_pointer ndArray, FSD::FastSamplingData const *fsd_data) {
    std::map<pv::ScalarType, FSD::type> pvToFBTypeMap = {{pv::ScalarType::pvByte, FSD::type_int8},
        {pv::ScalarType::pvUByte, FSD::type_uint8}, {pv::ScalarType::pvShort, FSD::type_int16},
        {pv::ScalarType::pvUShort, FSD::type_uint16}, {pv::ScalarType::pvInt, FSD::type_int32},
        {pv::ScalarType::pvUInt, FSD::type_uint32}, {pv::ScalarType::pvLong, FSD::type_int64},
        {pv::ScalarType::pvULong, FSD::type_uint64}, {pv::ScalarType::pvFloat, FSD::type_float32},
        {pv::ScalarType::pvDouble, FSD::type_float64} };
    
    auto tempPVScalarArray = ndArray->getValue()->get<pv::PVScalarArray>();
    
    auto pvElemType = tempPVScalarArray->getScalarArray()->getElementType();
    auto nrOfElements = tempPVScalarArray->getLength();
    
    
    ASSERT_NE(pvToFBTypeMap.find(pvElemType), pvToFBTypeMap.end());
    
    EXPECT_EQ(pvToFBTypeMap[pvElemType], fsd_data->data_type());
    
    EXPECT_EQ(GetNrOfElements(fsd_data), nrOfElements);
    
    CompareElements(tempPVScalarArray, fsd_data);
}

template<typename scalarArrType>
pv::PVStructure::shared_pointer CreateTestNTNDArray(std::vector<int> dims) {
    pvNT::NTNDArrayBuilderPtr builder = pvNT::NTNDArray::createBuilder();
    
    //These are used to determine which union field to use
    pv::PVScalarArrayPtr typeTester = pv::getPVDataCreate()->createPVScalarArray<scalarArrType>();
    auto typeTesterField = typeTester->getField();
    
    //std::cout << "ID of test arr: " << someTestField->getID() << std::endl;
    
    size_t nrElements = 1;
    for (auto &i: dims) {
        nrElements *= i;
    }
    pvNT::NTNDArrayPtr ntndArray = builder->addTimeStamp()->create();
    typename scalarArrType::svector someValues;
    for (int i = 0; i < nrElements; i++) {
        someValues.push_back(i*2);
    }
    
    auto valueUnion = ntndArray->getValue();
    auto valueIntrospection = valueUnion->getUnion();
    bool fieldSelected = false;
    for (int i = 0; i < valueIntrospection->getNumberFields(); i++) {
        auto tempUnionField = valueIntrospection->getField(i);
        if (tempUnionField->getID() == typeTesterField->getID()) {
            valueUnion->select(i);
            fieldSelected = true;
            break;
        }
    }
    
    pv::PVFieldPtr selectedField = valueUnion->get(); //The field selected with select()
    dynamic_cast<scalarArrType*>(selectedField.get())->replace(freeze(someValues));
    
    
    pv::PVDataCreatePtr dataCreator;
    
    pv::PVStructureArray::shared_pointer dimsStruct = ntndArray->getDimension();
    
    pv::PVStructureArray::svector dimsVector(dims.size());
    
    pv::FieldBuilderPtr fb = pv::getFieldCreate()->createFieldBuilder();
    
    pv::StructureConstPtr dimensionStruct = dimsStruct->getStructureArray()->getStructure();
    
    for (int i = 0; i < dims.size(); i++) {
        dimsVector[i] = dataCreator->createPVStructure(dimensionStruct);
        dimsVector[i]->getSubField<pv::PVInt>("size")->put(dims[i]);
        dimsVector[i]->getSubField<pv::PVInt>("fullSize")->put(dims[i]);
        dimsVector[i]->getSubField<pv::PVInt>("binning")->put(1);
        dimsVector[i]->getSubField<pv::PVInt>("offset")->put(0);
        dimsVector[i]->getSubField<pv::PVBoolean>("reverse")->put(false);
    }
    dimsStruct->replace(freeze(dimsVector));
    
    pv::TimeStamp timeStamp;
    timeStamp.fromTime_t(time(nullptr));
    timeStamp += 0.123456789;
    pv::PVTimeStamp pvTimeStamp;
    ntndArray->attachDataTimeStamp(pvTimeStamp);
    pvTimeStamp.set(timeStamp);
    
    
    pv::TimeStamp timeStamp2;
    timeStamp2.fromTime_t(time(nullptr));
    timeStamp2 += 0.111111111111;
    pv::PVTimeStamp pvTimeStamp2;
    ntndArray->attachTimeStamp(pvTimeStamp2);
    pvTimeStamp2.set(timeStamp2);
    
    return ntndArray->getPVStructure();
}


TEST_F(NTNDArrayTestSetUp, NTNDArrayToFBSuccess) {
    auto testData = CreateTestNTNDArray<pv::PVIntArray>({200, 200});
    EXPECT_TRUE(ExtractNTNDArrayData(&fb_builder, testData));
}

TEST_F(NTNDArrayTestSetUp, NTNDArrayToTimeTest) {
    auto testData = CreateTestNTNDArray<pv::PVIntArray>({10,10});
    ExtractNTNDArrayData(&fb_builder, testData);
    auto fbResult = FSD::GetFastSamplingData(fb_builder.GetBufferPointer());
    auto tempNTNDArray = pvNT::NTNDArray::wrap(testData);
    pv::PVTimeStamp timeStamp;
    timeStamp.attach(tempNTNDArray->getDataTimeStamp());
    EPICS_To_FB_TimeComparison(timeStamp, fbResult);
}

TEST_F(NTNDArrayTestSetUp, NTNDArrayToValueTest) {
    auto testData = CreateTestNTNDArray<pv::PVIntArray>({10, 10});
    ExtractNTNDArrayData(&fb_builder, testData);
    auto fbResult = FSD::GetFastSamplingData(fb_builder.GetBufferPointer());
    auto tempNTNDArray = pvNT::NTNDArray::wrap(testData);
    EPICS_To_FB_ValueComparison(tempNTNDArray, fbResult);
}

template <typename scalarArrType>
void TestNTNDArrConversion() {
    flatbuffers::FlatBufferBuilder local_fb_builder;
    auto testData = CreateTestNTNDArray<scalarArrType>({3,5,7});
    ExtractNTNDArrayData(&local_fb_builder, testData);
    auto fbResult = FSD::GetFastSamplingData(local_fb_builder.GetBufferPointer());
    auto tempNTNDArray = pvNT::NTNDArray::wrap(testData);
    EPICS_To_FB_ValueComparison(tempNTNDArray, fbResult);
}

TEST_F(NTNDArrayTestSetUp, NTNDArrayAllTypesTest) {
    TestNTNDArrConversion<pv::PVByteArray>();
    TestNTNDArrConversion<pv::PVUByteArray>();
    
    TestNTNDArrConversion<pv::PVShortArray>();
    TestNTNDArrConversion<pv::PVUShortArray>();
    
    TestNTNDArrConversion<pv::PVIntArray>();
    TestNTNDArrConversion<pv::PVUIntArray>();
    
    TestNTNDArrConversion<pv::PVLongArray>();
    TestNTNDArrConversion<pv::PVULongArray>();
    
    TestNTNDArrConversion<pv::PVFloatArray>();
    TestNTNDArrConversion<pv::PVDoubleArray>();
}

TEST_F(NTNDArrayTestSetUp, TestDimensions) {
    auto testData = CreateTestNTNDArray<pv::PVDoubleArray>({11, 12, 13});
    ExtractNTNDArrayData(&fb_builder, testData);
    auto fbResult = FSD::GetFastSamplingData(fb_builder.GetBufferPointer());
    auto fbValueArr = static_cast<const FSD::float64*>(fbResult->data());
    std::uint64_t elemFromFBDims = 1;
    for (auto iter = fbResult->dimensions()->begin(); iter != fbResult->dimensions()->end(); iter++) {
        elemFromFBDims *= *iter;
    }
    
    EXPECT_EQ(elemFromFBDims, fbValueArr->value()->size());
    auto tempNTNDArray = pvNT::NTNDArray::wrap(testData);
    auto tempPVScalarArray = dynamic_cast<const pv::PVScalarArray*>(tempNTNDArray->getValue()->get().get());
    
    std::uint64_t elemFromNDDims = 1;
    auto dimsNDVector = tempNTNDArray->getDimension()->view();
    for (int j = 0; j < dimsNDVector.size(); j++) {
        elemFromNDDims *= dimsNDVector[j]->getSubField<pv::PVInt>("size")->get();
    }
    EXPECT_EQ(elemFromFBDims, elemFromNDDims);
    
    auto PVnrOfElements = tempPVScalarArray->getLength();
    EXPECT_EQ(PVnrOfElements, elemFromNDDims);
    EXPECT_EQ(elemFromFBDims, PVnrOfElements);
}
