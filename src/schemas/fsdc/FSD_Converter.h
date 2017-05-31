#pragma once

#include "logger.h"
#include "epics-to-fb.h"
#include "MakeFlatBufferFromPVStructure.h"
#include "SchemaRegistry.h"

using namespace BrightnESS::FlatBufs;

typedef BrightnESS::FlatBufs::FB FB_BuilderContainer;

class FSD_Converter : public MakeFlatBufferFromPVStructure {
public:
    FSD_Converter();
    FB_uptr convert(EpicsPVUpdate const & pvData) override;
};

std::uint64_t GetNSecTimeStamp(epics::pvData::PVStructure::shared_pointer pvData);

bool ExtractNTScalarArrayData(flatbuffers::FlatBufferBuilder *builder, epics::pvData::PVStructure::shared_pointer pvData);
bool ExtractNTNDArrayData(flatbuffers::FlatBufferBuilder *builder, epics::pvData::PVStructure::shared_pointer pvData);
bool ExtractIfcdaqData(flatbuffers::FlatBufferBuilder *builder, epics::pvData::PVStructure::shared_pointer pvData, const std::string &pvName);


class Info : public SchemaInfo {
public:
    MakeFlatBufferFromPVStructure::ptr create_converter() override;
};
