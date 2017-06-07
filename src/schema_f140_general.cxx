#include "logger.h"
#include "SchemaRegistry.h"
#include "schemas/f140_general_generated.h"
#include "epics-to-fb.h"
#include "epics-pvstr.h"

namespace BrightnESS {
namespace FlatBufs {
namespace f140 {

#define DO_FLOG 0
#if DO_FLOG
using fmt::print;
#ifdef _MSC_VER
#define FLOG(level, fmt, ...)                                                  \
  print("{:{}s}" fmt "\n", "", 2 * (level), __VA_ARGS__);
#else
#define FLOG(level, fmt, args...)                                              \
  print("{:{}s}" fmt "\n", "", 2 * (level), ##args);
#endif
#else
#define FLOG(level, fmt, ...)
#endif

namespace fbg {

using std::vector;
using fmt::print;
// using F = FlatBufs::f140_general::F;
using namespace FlatBufs::f140_general;
// using ObjM = FlatBufs::f140_general::ObjM;
using epics::pvData::PVStructure;
typedef struct {
  F type;
  flatbuffers::Offset<void> off;
} F_t;

F_t Field(flatbuffers::FlatBufferBuilder &builder,
          epics::pvData::PVField const *field, int level);

inline static F_t field_PVStructure(flatbuffers::FlatBufferBuilder &builder,
                                    PVStructure const *field, int level) {
  auto &subfields = field->getPVFields();
  FLOG(level, "structure  subfields.size(): {}", subfields.size());

  // For each subfield, collect the offsets:
  vector<F_t> fs;
  for (auto &f1ptr : subfields) {
    fs.push_back(Field(builder, f1ptr.get(), 1 + level));
  }

#if DO_FLOG
  for (auto &x : fs) {
    FLOG(level, "off: {:d}", x.off.o);
  }
#endif

  // With the collected offsets, create object members
  // Collect raw vector of offsets to store later in flat buffer
  vector<flatbuffers::Offset<ObjM> > f2;
  for (auto &x : fs) {
    ObjMBuilder b1(builder);
    b1.add_v_type(x.type);
    b1.add_v(x.off);
    f2.push_back(b1.Finish());
  }
  auto v1 = builder.CreateVector(f2);

  ObjBuilder bo(builder);
  bo.add_ms(v1);
  return { F::Obj, bo.Finish().Union() };
}

inline static F_t field_PVStructure_array(
    flatbuffers::FlatBufferBuilder &builder,
    epics::pvData::PVValueArray<epics::pvData::PVStructurePtr> const *field,
    int level) {
  auto view = field->view();
  FLOG(level, "structureArray  [size(): {}]", view.size());
  vector<flatbuffers::Offset<Obj> > v1;
  for (auto &x : view) {
    FLOG(level, "entry");
    auto sub = Field(builder, x.get(), 1 + level);
    if (sub.type != F::Obj) {
      FLOG(level, "ERROR mismatched types in the EPICS structure");
    }
    v1.push_back(sub.off.o);
  }
  auto v2 = builder.CreateVector(v1);
  Obj_aBuilder b(builder);
  b.add_v(v2);
  return { F::Obj_a, b.Finish().Union() };
}

inline static F_t field_PVScalar(flatbuffers::FlatBufferBuilder &builder,
                                 epics::pvData::PVScalar const *field,
                                 int level) {
  FLOG(level, "scalar");
  auto stype = field->getScalar()->getScalarType();
#define M(T, B, E)                                                             \
  if (stype == epics::pvData::ScalarType::E) {                                 \
    auto p1 =                                                                  \
        reinterpret_cast<epics::pvData::PVScalarValue<T> const *>(field);      \
    B b(builder);                                                              \
    b.add_v(p1->get());                                                        \
    auto off = b.Finish().Union();                                             \
    FLOG(level, "off: {}  v: {}", off.o, p1->get());                           \
    return { F::E, off };                                                      \
  }
  M(int8_t, pvByteBuilder, pvByte);
  M(int16_t, pvShortBuilder, pvShort);
  M(int32_t, pvIntBuilder, pvInt);
  M(int64_t, pvLongBuilder, pvLong);
  M(uint8_t, pvUByteBuilder, pvUByte);
  M(uint16_t, pvUShortBuilder, pvUShort);
  M(uint32_t, pvUIntBuilder, pvUInt);
  M(uint64_t, pvULongBuilder, pvULong);
  M(float, pvFloatBuilder, pvFloat);
  M(double, pvDoubleBuilder, pvDouble);
#undef M
  if (stype == epics::pvData::ScalarType::pvString) {
    auto p1 =
        reinterpret_cast<epics::pvData::PVScalarValue<std::string> const *>(
            field);
    auto s1 = builder.CreateString(p1->get());
    pvStringBuilder b(builder);
    b.add_v(s1);
    return { F::pvString, b.Finish().Union() };
  }
  if (stype == epics::pvData::ScalarType::pvBoolean) {
    FLOG(level, "WARNING boolean handled as byte");
    auto p1 =
        reinterpret_cast<epics::pvData::PVScalarValue<bool> const *>(field);
    pvByteBuilder b(builder);
    b.add_v(p1->get());
    auto off = b.Finish().Union();
    FLOG(level, "off: {}", off.o);
    return { F::pvByte, off };
  }
  return { F::NONE, 0 };
}

inline static F_t
field_PVScalar_array(flatbuffers::FlatBufferBuilder &builder,
                     epics::pvData::PVScalarArray const *field, int level) {
  FLOG(level, "scalar array");
  auto stype = field->getScalarArray()->getElementType();
// DT * a1 = nullptr;
// auto v1 = ret.builder->CreateUninitializedVector(size, sizeof(DT),
// (uint8_t**)&a1);
#define M(TC, TB, TF, TE)                                                      \
  if (stype == epics::pvData::ScalarType::TE) {                                \
    auto p1 =                                                                  \
        reinterpret_cast<epics::pvData::PVValueArray<TC> const *>(field);      \
    auto view = p1->view();                                                    \
    auto v1 = builder.CreateVector(view.data(), view.size());                  \
    TB b(builder);                                                             \
    b.add_v(v1);                                                               \
    return { F::TF, b.Finish().Union() };                                      \
  }
  M(int8_t, pvByte_aBuilder, pvByte_a, pvByte);
  M(int16_t, pvShort_aBuilder, pvShort_a, pvShort);
  M(int32_t, pvInt_aBuilder, pvInt_a, pvInt);
  M(int64_t, pvLong_aBuilder, pvLong_a, pvLong);
  M(uint8_t, pvUByte_aBuilder, pvUByte_a, pvUByte);
  M(uint16_t, pvUShort_aBuilder, pvUShort_a, pvUShort);
  M(uint32_t, pvUInt_aBuilder, pvUInt_a, pvUInt);
  M(uint64_t, pvULong_aBuilder, pvULong_a, pvULong);
  M(float, pvFloat_aBuilder, pvFloat_a, pvFloat);
  M(double, pvDouble_aBuilder, pvDouble_a, pvDouble);
#undef M

  if (stype == epics::pvData::ScalarType::pvString) {
    auto p1 =
        reinterpret_cast<epics::pvData::PVValueArray<std::string> const *>(
            field);
    FLOG(level, "WARNING serializing string arrays is disabled...");
    return { F::NONE, 0 };
    auto view = p1->view();
    vector<flatbuffers::Offset<flatbuffers::String> > v1;
    for (auto &s0 : view) {
      v1.push_back(builder.CreateString(s0));
    }
    auto v2 = builder.CreateVector(v1);
    pvString_aBuilder b(builder);
    b.add_v(v2);
    return { F::pvString_a, b.Finish().Union() };
  }
  if (stype == epics::pvData::ScalarType::pvBoolean) {
    FLOG(level, "WARNING array of booleans are not handled so far");
    return { F::NONE, 0 };
  }
  return { F::NONE, 0 };
}

inline static F_t field_PVUnion(flatbuffers::FlatBufferBuilder &builder,
                                epics::pvData::PVUnion const *field,
                                int level) {
  FLOG(level, "union");
  auto f3 = field->get();
  if (f3) {
    return Field(builder, f3.get(), 1 + level);
  }
  // The union does not contain anything:
  return { F::NONE, 0 };
}

F_t Field(flatbuffers::FlatBufferBuilder &builder,
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
    return { F::NONE, 0 };
  }

  FLOG(level, "ERROR unknown type");
  return { F::NONE, 0 };
}

F_t Field(flatbuffers::FlatBufferBuilder &builder,
          epics::pvData::PVFieldPtr const &field, int level) {
  return Field(builder, field.get(), level);
}
}

class Converter : public MakeFlatBufferFromPVStructure {
public:
  BrightnESS::FlatBufs::FB_uptr convert(EpicsPVUpdate const &up) override {
    // Passing initial size:
    auto &pvstr = up.epics_pvstr;
    auto fb = BrightnESS::FlatBufs::FB_uptr(new BrightnESS::FlatBufs::FB);
    uint64_t ts_data = 0;
    if (auto x =
            pvstr->getSubField<epics::pvData::PVScalarValue<uint64_t> >("ts")) {
      ts_data = x->get();
    }
    fb->seq = up.seq;
    fb->fwdix = up.fwdix;
    auto builder = fb->builder.get();
    auto n = builder->CreateString("some-name-must-go-here");
    auto vF = fbg::Field(*builder, pvstr, 0);
    // some kind of 'union F' offset:   flatbuffers::Offset<void>
    FlatBufs::f140_general::PVBuilder b(*builder);
    b.add_n(n);
    b.add_v_type(vF.type);
    auto fi = FlatBufs::f140_general::fwdinfo_t(up.seq, ts_data,
                                                up.ts_epics_monitor, up.fwdix);
    b.add_fwdinfo(&fi);
    FinishPVBuffer(*builder, b.Finish());
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

FlatBufs::SchemaRegistry::Registrar<Info> g_registrar_info("f140",
                                                           Info::ptr(new Info));
}
}
}
