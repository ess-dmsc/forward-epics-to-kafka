#include "SchemaRegistry.h"
#include "epics-to-fb.h"
#include "helper.h"
#include "logger.h"
#include "schemas/f141_epics_nt_generated.h"

namespace BrightnESS {
namespace FlatBufs {
namespace f141 {

#define STRINGIFY2(x) #x
#define STRINGIFY(x) STRINGIFY2(x)

template <typename T> char const *type_name();
// template <> char const * type_name<uint32_t>() { return "uint32_t"; }
// Unstringification not possible, so not possible to give white space..
#define M(x)                                                                   \
  template <> char const *type_name<x>() { return STRINGIFY(x); }
M(int8_t)
M(uint8_t)
M(int16_t)
M(uint16_t)
M(int32_t)
M(uint32_t)
M(int64_t)
M(uint64_t)
M(float)
M(double)
#undef M

using namespace FlatBufs::f141_epics_nt;

typedef struct {
  PV type;
  flatbuffers::Offset<void> off;
} PV_t;

namespace PVStructureToFlatBufferN {

// using PVE = BrightnESS::FlatBufs::f141_epics_nt::PV;
using namespace BrightnESS::FlatBufs::f141_epics_nt;

struct Enum_PV_Base {};

template <typename T0> struct BuilderType_to_Enum_PV : public Enum_PV_Base {
  static PV v();
};
template <>
struct BuilderType_to_Enum_PV<NTScalarByteBuilder> : public Enum_PV_Base {
  static PV v() { return PV::NTScalarByte; }
};
template <>
struct BuilderType_to_Enum_PV<NTScalarUByteBuilder> : public Enum_PV_Base {
  static PV v() { return PV::NTScalarUByte; }
};
template <>
struct BuilderType_to_Enum_PV<NTScalarShortBuilder> : public Enum_PV_Base {
  static PV v() { return PV::NTScalarShort; }
};
template <>
struct BuilderType_to_Enum_PV<NTScalarUShortBuilder> : public Enum_PV_Base {
  static PV v() { return PV::NTScalarUShort; }
};
template <>
struct BuilderType_to_Enum_PV<NTScalarIntBuilder> : public Enum_PV_Base {
  static PV v() { return PV::NTScalarInt; }
};
template <>
struct BuilderType_to_Enum_PV<NTScalarUIntBuilder> : public Enum_PV_Base {
  static PV v() { return PV::NTScalarUInt; }
};
template <>
struct BuilderType_to_Enum_PV<NTScalarLongBuilder> : public Enum_PV_Base {
  static PV v() { return PV::NTScalarLong; }
};
template <>
struct BuilderType_to_Enum_PV<NTScalarULongBuilder> : public Enum_PV_Base {
  static PV v() { return PV::NTScalarULong; }
};
template <>
struct BuilderType_to_Enum_PV<NTScalarFloatBuilder> : public Enum_PV_Base {
  static PV v() { return PV::NTScalarFloat; }
};
template <>
struct BuilderType_to_Enum_PV<NTScalarDoubleBuilder> : public Enum_PV_Base {
  static PV v() { return PV::NTScalarDouble; }
};

template <>
struct BuilderType_to_Enum_PV<NTScalarArrayByteBuilder> : public Enum_PV_Base {
  static PV v() { return PV::NTScalarArrayByte; }
};
template <>
struct BuilderType_to_Enum_PV<NTScalarArrayUByteBuilder> : public Enum_PV_Base {
  static PV v() { return PV::NTScalarArrayUByte; }
};
template <>
struct BuilderType_to_Enum_PV<NTScalarArrayShortBuilder> : public Enum_PV_Base {
  static PV v() { return PV::NTScalarArrayShort; }
};
template <>
struct BuilderType_to_Enum_PV<NTScalarArrayUShortBuilder>
    : public Enum_PV_Base {
  static PV v() { return PV::NTScalarArrayUShort; }
};
template <>
struct BuilderType_to_Enum_PV<NTScalarArrayIntBuilder> : public Enum_PV_Base {
  static PV v() { return PV::NTScalarArrayInt; }
};
template <>
struct BuilderType_to_Enum_PV<NTScalarArrayUIntBuilder> : public Enum_PV_Base {
  static PV v() { return PV::NTScalarArrayUInt; }
};
template <>
struct BuilderType_to_Enum_PV<NTScalarArrayLongBuilder> : public Enum_PV_Base {
  static PV v() { return PV::NTScalarArrayLong; }
};
template <>
struct BuilderType_to_Enum_PV<NTScalarArrayULongBuilder> : public Enum_PV_Base {
  static PV v() { return PV::NTScalarArrayULong; }
};
template <>
struct BuilderType_to_Enum_PV<NTScalarArrayFloatBuilder> : public Enum_PV_Base {
  static PV v() { return PV::NTScalarArrayFloat; }
};
template <>
struct BuilderType_to_Enum_PV<NTScalarArrayDoubleBuilder>
    : public Enum_PV_Base {
  static PV v() { return PV::NTScalarArrayDouble; }
};

template <typename T0> class Make_Scalar {
public:
  // clang-format off
  using T1 =
    typename std::conditional<std::is_same<T0, epics::pvData::boolean>::value, NTScalarByteBuilder,
    typename std::conditional<std::is_same<T0,   int8_t>::value, NTScalarByteBuilder,
    typename std::conditional<std::is_same<T0,  int16_t>::value, NTScalarShortBuilder,
    typename std::conditional<std::is_same<T0,  int32_t>::value, NTScalarIntBuilder,
    typename std::conditional<std::is_same<T0,  int64_t>::value, NTScalarLongBuilder,
    typename std::conditional<std::is_same<T0,  uint8_t>::value, NTScalarUByteBuilder,
    typename std::conditional<std::is_same<T0, uint16_t>::value, NTScalarUShortBuilder,
    typename std::conditional<std::is_same<T0, uint32_t>::value, NTScalarUIntBuilder,
    typename std::conditional<std::is_same<T0, uint64_t>::value, NTScalarULongBuilder,
    typename std::conditional<std::is_same<T0,    float>::value, NTScalarFloatBuilder,
    typename std::conditional<std::is_same<T0,   double>::value, NTScalarDoubleBuilder,
    std::nullptr_t>::type>::type>::type>::type>::type>::type>::type>::type>::type>::type>::type;
  // clang-format on

  static PV_t convert(flatbuffers::FlatBufferBuilder *builder,
                      epics::pvData::PVScalar *field_) {
    auto field = static_cast<epics::pvData::PVScalarValue<T0> *>(field_);
    T0 val = field->get();
    T1 pv_builder(*builder);
    pv_builder.add_value(val);
    return {BuilderType_to_Enum_PV<T1>::v(), pv_builder.Finish().Union()};
  }
};

template <typename T0> class Make_ScalarArray {
public:
  // clang-format off
  using T1 =
    typename std::conditional<std::is_same<T0, epics::pvData::boolean>::value, NTScalarArrayByteBuilder,
    typename std::conditional<std::is_same<T0,   int8_t>::value, NTScalarArrayByteBuilder,
    typename std::conditional<std::is_same<T0,  int16_t>::value, NTScalarArrayShortBuilder,
    typename std::conditional<std::is_same<T0,  int32_t>::value, NTScalarArrayIntBuilder,
    typename std::conditional<std::is_same<T0,  int64_t>::value, NTScalarArrayLongBuilder,
    typename std::conditional<std::is_same<T0,  uint8_t>::value, NTScalarArrayUByteBuilder,
    typename std::conditional<std::is_same<T0, uint16_t>::value, NTScalarArrayUShortBuilder,
    typename std::conditional<std::is_same<T0, uint32_t>::value, NTScalarArrayUIntBuilder,
    typename std::conditional<std::is_same<T0, uint64_t>::value, NTScalarArrayULongBuilder,
    typename std::conditional<std::is_same<T0,    float>::value, NTScalarArrayFloatBuilder,
    typename std::conditional<std::is_same<T0,   double>::value, NTScalarArrayDoubleBuilder,
    std::nullptr_t>::type>::type>::type>::type>::type>::type>::type>::type>::type>::type>::type;

  using T3 = typename std::conditional<std::is_same<T0, epics::pvData::boolean>::value, signed char, T0>::type;
  // clang-format on

  static PV_t convert(flatbuffers::FlatBufferBuilder *builder,
                      epics::pvData::PVScalarArray *field_) {
    auto field = static_cast<epics::pvData::PVValueArray<T0> *>(field_);
    auto svec = field->view();
    auto nlen = svec.size();
    auto val = builder->CreateVector((T3 *)svec.data(), nlen);
    T1 pv_builder(*builder);
    pv_builder.add_value(val);
    return {BuilderType_to_Enum_PV<T1>::v(), pv_builder.Finish().Union()};
  }
};

} // end namespace PVStructureToFlatBufferN

PV_t make_PV_scalar(flatbuffers::FlatBufferBuilder &builder,
                    epics::pvData::PVScalar *field) {
  using S = epics::pvData::ScalarType;
  using namespace epics::pvData;
  using namespace PVStructureToFlatBufferN;
  switch (field->getScalar()->getScalarType()) {
  case S::pvBoolean:
    return Make_Scalar<epics::pvData::boolean>::convert(&builder, field);
  case S::pvByte:
    return Make_Scalar<int8_t>::convert(&builder, field);
  case S::pvShort:
    return Make_Scalar<int16_t>::convert(&builder, field);
  case S::pvInt:
    return Make_Scalar<int32_t>::convert(&builder, field);
  case S::pvLong:
    return Make_Scalar<int64_t>::convert(&builder, field);
  case S::pvUByte:
    return Make_Scalar<uint8_t>::convert(&builder, field);
  case S::pvUShort:
    return Make_Scalar<uint16_t>::convert(&builder, field);
  case S::pvUInt:
    return Make_Scalar<uint32_t>::convert(&builder, field);
  case S::pvULong:
    return Make_Scalar<uint64_t>::convert(&builder, field);
  case S::pvFloat:
    return Make_Scalar<float>::convert(&builder, field);
  case S::pvDouble:
    return Make_Scalar<double>::convert(&builder, field);
  case S::pvString:
    // Sorry, not implemented yet
    LOG(5, "ERROR pvString not implemented yet");
    break;
  }
  return {PV::NTScalarByte, 0};
}

PV_t make_PV_scalar_array(flatbuffers::FlatBufferBuilder &builder,
                          epics::pvData::PVScalarArray *field) {
  using S = epics::pvData::ScalarType;
  using namespace epics::pvData;
  using namespace PVStructureToFlatBufferN;
  switch (field->getScalarArray()->getElementType()) {
  case S::pvBoolean:
    return Make_ScalarArray<epics::pvData::boolean>::convert(&builder, field);
  case S::pvByte:
    return Make_ScalarArray<int8_t>::convert(&builder, field);
  case S::pvShort:
    return Make_ScalarArray<int16_t>::convert(&builder, field);
  case S::pvInt:
    return Make_ScalarArray<int32_t>::convert(&builder, field);
  case S::pvLong:
    return Make_ScalarArray<int64_t>::convert(&builder, field);
  case S::pvUByte:
    return Make_ScalarArray<uint8_t>::convert(&builder, field);
  case S::pvUShort:
    return Make_ScalarArray<uint16_t>::convert(&builder, field);
  case S::pvUInt:
    return Make_ScalarArray<uint32_t>::convert(&builder, field);
  case S::pvULong:
    return Make_ScalarArray<uint64_t>::convert(&builder, field);
  case S::pvFloat:
    return Make_ScalarArray<float>::convert(&builder, field);
  case S::pvDouble:
    return Make_ScalarArray<double>::convert(&builder, field);
  case S::pvString:
    // Sorry, not implemented yet
    LOG(5, "ERROR pvString not implemented yet");
    break;
  }
  return {PV::NTScalarByte, 0};
}

PV_t make_PV(flatbuffers::FlatBufferBuilder &builder,
             epics::pvData::PVFieldPtr const &field) {
  if (!field) {
    LOG(0, "ERROR can not do anything with a null pointer");
    return {PV::NTScalarByte, 0};
  }
  // Check the type of 'value'
  // Optionally, compare with name of the PV?
  // Create appropriate fb union
  // CreateVector using the correct types.
  auto t1 = field->getField()->getType();
  using T = epics::pvData::Type;
  switch (t1) {
  case T::scalar:
    return make_PV_scalar(builder,
                          static_cast<epics::pvData::PVScalar *>(field.get()));
  case T::scalarArray:
    return make_PV_scalar_array(
        builder, static_cast<epics::pvData::PVScalarArray *>(field.get()));
  case T::structure:
    LOG(5, "ERROR Type::structure can not be handled");
    break;
  case T::structureArray:
    LOG(5, "ERROR Type::structureArray can not be handled");
    break;
  case T::union_:
    LOG(5, "ERROR Type::union_ can not be handled");
    break;
  case T::unionArray:
    LOG(5, "ERROR Type::unionArray can not be handled");
    break;
  }
  return {PV::NTScalarByte, 0};
}

/// Schema f141 will be likely not used infavor of f142 and f143.
/// Discussing removal.

class Converter : public MakeFlatBufferFromPVStructure {
public:
  BrightnESS::FlatBufs::FlatbufferMessage::uptr
  convert(EpicsPVUpdate const &up) override {
    auto &pvstr = up.epics_pvstr;
    auto fb = make_unique<BrightnESS::FlatBufs::FlatbufferMessage>();
    uint64_t ts_data = 0;
    if (auto x =
            pvstr->getSubField<epics::pvData::PVScalarValue<uint64_t>>("ts")) {
      ts_data = x->get();
    }
    auto builder = fb->builder.get();
    // this is the field type ID string: up.pvstr->getStructure()->getID()
    auto n = builder->CreateString(up.channel);
    auto vF = make_PV(*builder, pvstr->getSubField("value"));
    // some kind of 'union F' offset:   flatbuffers::Offset<void>

    fwdinfo_2_tBuilder bf(*builder);
    uint64_t seq_data = 0;
    if (auto x =
            pvstr->getSubField<epics::pvData::PVScalarValue<uint64_t>>("seq")) {
      seq_data = x->get();
    }
    bf.add_seq_data(seq_data);
    bf.add_seq_fwd(up.seq_fwd);
    bf.add_ts_data(ts_data);
    bf.add_ts_fwd(up.ts_epics_monitor);
    auto fwdinfo2 = bf.Finish().Union();

    FlatBufs::f141_epics_nt::EpicsPVBuilder b(*builder);
    b.add_name(n);
    b.add_pv_type(vF.type);
    b.add_pv(vF.off);

    if (auto pvTimeStamp =
            pvstr->getSubField<epics::pvData::PVStructure>("timeStamp")) {
      timeStamp_t timeStamp(
          pvTimeStamp
              ->getSubField<epics::pvData::PVScalarValue<int64_t>>(
                  "secondsPastEpoch")
              ->get(),
          pvTimeStamp
              ->getSubField<epics::pvData::PVScalarValue<int32_t>>(
                  "nanoseconds")
              ->get());
      b.add_timeStamp(&timeStamp);
    } else {
      LOG(5, "timeStamp not available");
    }

    b.add_fwdinfo2_type(fwdinfo_u::fwdinfo_2_t);
    b.add_fwdinfo2(fwdinfo2);

    FinishEpicsPVBuffer(*builder, b.Finish());
    if (log_level <= 0) {
      auto b1 = binary_to_hex((char const *)builder->GetBufferPointer(),
                              builder->GetSize());
      LOG(7, "seq data/fwd: {} / {}  schema: [{}]\n{:.{}}", seq_data,
          up.seq_fwd, FlatBufs::f141_epics_nt::EpicsPVIdentifier(), b1.data(),
          b1.size());
    }
    return fb;
  }
};

class Info : public SchemaInfo {
public:
  MakeFlatBufferFromPVStructure::ptr create_converter() override;
};

MakeFlatBufferFromPVStructure::ptr Info::create_converter() {
  return MakeFlatBufferFromPVStructure::ptr(new Converter);
}

FlatBufs::SchemaRegistry::Registrar<Info> g_registrar_info("f141",
                                                           Info::ptr(new Info));
}
}
}
