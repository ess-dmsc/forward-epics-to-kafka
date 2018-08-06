#include "../../EpicsPVUpdate.h"
#include "../../FlatBufferCreator.h"
#include "../../SchemaRegistry.h"
#include "../../helper.h"
#include "../../logger.h"
#include "schemas/f143_structure_generated.h"

namespace FlatBufs {
namespace f143 {

#include <fmt/format.h>

#define DO_FLOG 1
#if DO_FLOG
using fmt::print;
#ifdef _MSC_VER
#define FLOG(level, fmt, ...)                                                  \
  if (level < 60) {                                                            \
    print("{:{}s}" fmt "\n", "", 2 * (level), __VA_ARGS__);                    \
  }
#else
#define FLOG(level, fmt, args...)                                              \
  if (level < 60) {                                                            \
    print("{:{}s}" fmt "\n", "", 2 * (level), ##args);                         \
  }
#endif
#else
#define FLOG(level, fmt, ...)
#endif

namespace fbg {

using fmt::print;
using std::string;
using std::vector;
using namespace f143_structure;
using epics::pvData::PVStructure;
typedef struct {
  Value type;
  flatbuffers::Offset<void> off;
} V_t;

V_t Field(flatbuffers::FlatBufferBuilder &builder,
          epics::pvData::PVField const *field, int level);

inline static V_t field_PVStructure(flatbuffers::FlatBufferBuilder &builder,
                                    PVStructure const *field, int level) {
  auto &subfields = field->getPVFields();
  FLOG(level, "structure  subfields.size(): {}", subfields.size());

  // For each subfield, collect the offsets:
  vector<string> names;
  vector<V_t> fs;
  for (auto &f1ptr : subfields) {
    names.push_back(f1ptr->getFieldName());
    auto v1 = Field(builder, f1ptr.get(), 1 + level);
    if (v1.type != Value::NONE) {
      fs.push_back(v1);
    }
  }

  // With the collected offsets, create object members
  // Collect raw vector of offsets to store later in flat buffer
  vector<flatbuffers::Offset<ObjM>> f2;
  uint32_t i1 = 0;
  for (auto &x : fs) {
    FLOG(level, "off: {:5d}  {}", x.off.o, names[i1]);
    auto n1 = builder.CreateString(names[i1]);
    ObjMBuilder b1(builder);
    b1.add_k(n1);
    b1.add_v_type(x.type);
    b1.add_v(x.off);
    f2.push_back(b1.Finish());
    ++i1;
  }
  auto v1 = builder.CreateVector(f2);

  ObjBuilder bo(builder);
  bo.add_value(v1);
  return {Value::Obj, bo.Finish().Union()};
}

inline static V_t field_PVStructure_array(
    flatbuffers::FlatBufferBuilder &builder,
    epics::pvData::PVValueArray<epics::pvData::PVStructurePtr> const *field,
    int level) {
  auto view = field->view();
  FLOG(level, "structureArray  [size(): {}]", view.size());
  vector<flatbuffers::Offset<Obj>> v1;
  for (auto &x : view) {
    FLOG(level, "entry");
    auto sub = Field(builder, x.get(), 1 + level);
    if (sub.type != Value::NONE) {
      if (sub.type != Value::Obj) {
        FLOG(level, "ERROR mismatched types in the EPICS structure");
      } else {
        v1.push_back(sub.off.o);
      }
    }
  }
  auto v2 = builder.CreateVector(v1);
  ArrayObjBuilder b(builder);
  b.add_value(v2);
  return {Value::ArrayObj, b.Finish().Union()};
}

inline static V_t field_PVScalar(flatbuffers::FlatBufferBuilder &builder,
                                 epics::pvData::PVScalar const *field,
                                 int level) {
  FLOG(level, "scalar");
  auto stype = field->getScalar()->getScalarType();
#define M(T, B, E, VT)                                                         \
  if (stype == epics::pvData::ScalarType::E) {                                 \
    auto p1 =                                                                  \
        reinterpret_cast<epics::pvData::PVScalarValue<T> const *>(field);      \
    B b(builder);                                                              \
    b.add_value(p1->get());                                                    \
    auto off = b.Finish().Union();                                             \
    FLOG(level, "off: {}  v: {}", off.o, p1->get());                           \
    return {Value::VT, off};                                                   \
  }
  M(int8_t, ByteBuilder, pvByte, Byte);
  M(int16_t, ShortBuilder, pvShort, Short);
  M(int32_t, IntBuilder, pvInt, Int);
  M(int64_t, LongBuilder, pvLong, Long);
  M(uint8_t, UByteBuilder, pvUByte, UByte);
  M(uint16_t, UShortBuilder, pvUShort, UShort);
  M(uint32_t, UIntBuilder, pvUInt, UInt);
  M(uint64_t, ULongBuilder, pvULong, ULong);
  M(float, FloatBuilder, pvFloat, Float);
  M(double, DoubleBuilder, pvDouble, Double);
#undef M
  if (stype == epics::pvData::ScalarType::pvString) {
    auto p1 =
        reinterpret_cast<epics::pvData::PVScalarValue<std::string> const *>(
            field);
    auto s1 = builder.CreateString(p1->get());
    StringBuilder b(builder);
    b.add_value(s1);
    return {Value::String, b.Finish().Union()};
  }
  if (stype == epics::pvData::ScalarType::pvBoolean) {
    FLOG(level, "WARNING boolean handled as byte");
    auto p1 =
        reinterpret_cast<epics::pvData::PVScalarValue<bool> const *>(field);
    ByteBuilder b(builder);
    b.add_value(p1->get());
    auto off = b.Finish().Union();
    FLOG(level, "off: {}", off.o);
    return {Value::Byte, off};
  }
  return {Value::NONE, 0};
}

inline static V_t
field_PVScalar_array(flatbuffers::FlatBufferBuilder &builder,
                     epics::pvData::PVScalarArray const *field, int level) {
  FLOG(level, "scalar array");
  auto stype = field->getScalarArray()->getElementType();
#define M(TC, TB, TF, TE)                                                      \
  if (stype == epics::pvData::ScalarType::TE) {                                \
    auto p1 =                                                                  \
        reinterpret_cast<epics::pvData::PVValueArray<TC> const *>(field);      \
    auto view = p1->view();                                                    \
    TC *a1 = nullptr;                                                          \
    auto v1 = builder.CreateUninitializedVector(view.size(), sizeof(TC),       \
                                                (uint8_t **)&a1);              \
    memcpy(a1, view.data(), sizeof(TC) * view.size());                         \
    TB b(builder);                                                             \
    b.add_value(v1);                                                           \
    return {Value::TF, b.Finish().Union()};                                    \
  }
  M(int8_t, ArrayByteBuilder, ArrayByte, pvByte);
  M(int16_t, ArrayShortBuilder, ArrayShort, pvShort);
  M(int32_t, ArrayIntBuilder, ArrayInt, pvInt);
  M(int64_t, ArrayLongBuilder, ArrayLong, pvLong);
  M(uint8_t, ArrayUByteBuilder, ArrayUByte, pvUByte);
  M(uint16_t, ArrayUShortBuilder, ArrayUShort, pvUShort);
  M(uint32_t, ArrayUIntBuilder, ArrayUInt, pvUInt);
  M(uint64_t, ArrayULongBuilder, ArrayULong, pvULong);
  M(float, ArrayFloatBuilder, ArrayFloat, pvFloat);
  M(double, ArrayDoubleBuilder, ArrayDouble, pvDouble);
#undef M

  if (stype == epics::pvData::ScalarType::pvString) {
    FLOG(level, "WARNING serializing string arrays is disabled...");
    return {Value::NONE, 0};
  }
  if (stype == epics::pvData::ScalarType::pvBoolean) {
    FLOG(level, "WARNING array of booleans are not handled so far");
    return {Value::NONE, 0};
  }
  return {Value::NONE, 0};
}

inline static V_t field_PVUnion(flatbuffers::FlatBufferBuilder &builder,
                                epics::pvData::PVUnion const *field,
                                int level) {
  FLOG(level, "union");
  auto f3 = field->get();
  if (f3) {
    return Field(builder, f3.get(), 1 + level);
  }
  // The union does not contain anything:
  return {Value::NONE, 0};
}

V_t Field(flatbuffers::FlatBufferBuilder &builder,
          epics::pvData::PVField const *field, int level) {
  FLOG(level, "N: {}", field->getFieldName());
  auto etype = field->getField()->getType();
  if (etype == epics::pvData::Type::structure) {
    return field_PVStructure(
        builder, reinterpret_cast<epics::pvData::PVStructure const *>(field),
        level + 1);
  } else if (etype == epics::pvData::Type::structureArray) {
    // Serialize all objects, collect the offsets, and store an array of those.
    return field_PVStructure_array(
        builder,
        reinterpret_cast<
            epics::pvData::PVValueArray<epics::pvData::PVStructurePtr> const *>(
            field),
        level + 1);
  } else if (etype == epics::pvData::Type::scalar) {
    return field_PVScalar(
        builder, reinterpret_cast<epics::pvData::PVScalar const *>(field),
        level + 1);
  } else if (etype == epics::pvData::Type::scalarArray) {
    return field_PVScalar_array(
        builder, reinterpret_cast<epics::pvData::PVScalarArray const *>(field),
        level + 1);
  } else if (etype == epics::pvData::Type::union_) {
    return field_PVUnion(
        builder, reinterpret_cast<epics::pvData::PVUnion const *>(field),
        level + 1);
  } else if (etype == epics::pvData::Type::unionArray) {
    FLOG(level, "union array not yet supported");
    return {Value::NONE, 0};
  }

  FLOG(level, "ERROR unknown type");
  return {Value::NONE, 0};
}

V_t Field(flatbuffers::FlatBufferBuilder &builder,
          epics::pvData::PVFieldPtr const &field, int level) {
  return Field(builder, field.get(), level);
}
} // namespace fbg

class Converter : public FlatBufferCreator {
public:
  std::unique_ptr<FlatBufs::FlatbufferMessage>
  create(EpicsPVUpdate const &up) override {
    // Passing initial size:
    auto &pvstr = up.epics_pvstr;
    auto fb = make_unique<FlatBufs::FlatbufferMessage>();
    auto builder = fb->builder.get();

    auto n = builder->CreateString(up.channel);
    auto vF = fbg::Field(*builder, pvstr, llevel);
    f143_structure::StructureBuilder b(*builder);
    b.add_name(n);
    b.add_value_type(vF.type);
    b.add_value(vF.off);
    if (auto pvTimeStamp =
            pvstr->getSubField<epics::pvData::PVStructure>("timeStamp")) {
      uint64_t ts = (uint64_t)pvTimeStamp
                        ->getSubField<epics::pvData::PVScalarValue<int64_t>>(
                            "secondsPastEpoch")
                        ->get();
      ts *= 1000000000;
      ts += pvTimeStamp
                ->getSubField<epics::pvData::PVScalarValue<int32_t>>(
                    "nanoseconds")
                ->get();
      b.add_timestamp(ts);
    }
    FinishStructureBuffer(*builder, b.Finish());
    return fb;
  }

  void config(std::map<std::string, int64_t> const &config_ints,
              std::map<std::string, std::string> const & /* config_strings */)
      override {
    auto it = config_ints.find("llevel");
    if (it != config_ints.end()) {
      llevel = it->second;
    }
  }
  int llevel = 1000;
};

class Info : public SchemaInfo {
public:
  std::unique_ptr<FlatBufferCreator> create_converter() override;
};

std::unique_ptr<FlatBufferCreator> Info::create_converter() {
  return make_unique<Converter>();
}

FlatBufs::SchemaRegistry::Registrar<Info> g_registrar_info("f143",
                                                           Info::ptr(new Info));
} // namespace f143
} // namespace FlatBufs
