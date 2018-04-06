#include "../../RangeSet.h"
#include "../../SchemaRegistry.h"
#include "../../epics-pvstr.h"
#include "../../epics-to-fb.h"
#include "../../helper.h"
#include "../../logger.h"
#include "schemas/f142_logdata_generated.h"
#include <atomic>
#include <mutex>
#include <pv/nt.h>
#include <pv/ntndarray.h>
#include <pv/ntndarrayAttribute.h>
#include <pv/ntutils.h>
#include <pv/pvEnumerated.h>
#include <set>

namespace BrightnESS {
namespace FlatBufs {
namespace f142 {

using std::string;

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
M(string)
#undef M

typedef struct {
  Value type;
  flatbuffers::Offset<void> off;
} Value_t;

struct Statistics {
  uint64_t err_timestamp_not_available = 0;
  uint64_t err_not_implemented_yet = 0;
};

namespace PVStructureToFlatBufferN {

struct Enum_Value_Base {};

template <typename T0>
struct BuilderType_to_Enum_Value : public Enum_Value_Base {
  static Value v();
};
template <>
struct BuilderType_to_Enum_Value<ByteBuilder> : public Enum_Value_Base {
  static Value v() { return Value::Byte; }
};
template <>
struct BuilderType_to_Enum_Value<UByteBuilder> : public Enum_Value_Base {
  static Value v() { return Value::UByte; }
};
template <>
struct BuilderType_to_Enum_Value<ShortBuilder> : public Enum_Value_Base {
  static Value v() { return Value::Short; }
};
template <>
struct BuilderType_to_Enum_Value<UShortBuilder> : public Enum_Value_Base {
  static Value v() { return Value::UShort; }
};
template <>
struct BuilderType_to_Enum_Value<IntBuilder> : public Enum_Value_Base {
  static Value v() { return Value::Int; }
};
template <>
struct BuilderType_to_Enum_Value<UIntBuilder> : public Enum_Value_Base {
  static Value v() { return Value::UInt; }
};
template <>
struct BuilderType_to_Enum_Value<LongBuilder> : public Enum_Value_Base {
  static Value v() { return Value::Long; }
};
template <>
struct BuilderType_to_Enum_Value<ULongBuilder> : public Enum_Value_Base {
  static Value v() { return Value::ULong; }
};
template <>
struct BuilderType_to_Enum_Value<FloatBuilder> : public Enum_Value_Base {
  static Value v() { return Value::Float; }
};
template <>
struct BuilderType_to_Enum_Value<DoubleBuilder> : public Enum_Value_Base {
  static Value v() { return Value::Double; }
};

template <>
struct BuilderType_to_Enum_Value<ArrayByteBuilder> : public Enum_Value_Base {
  static Value v() { return Value::ArrayByte; }
};
template <>
struct BuilderType_to_Enum_Value<ArrayUByteBuilder> : public Enum_Value_Base {
  static Value v() { return Value::ArrayUByte; }
};
template <>
struct BuilderType_to_Enum_Value<ArrayShortBuilder> : public Enum_Value_Base {
  static Value v() { return Value::ArrayShort; }
};
template <>
struct BuilderType_to_Enum_Value<ArrayUShortBuilder> : public Enum_Value_Base {
  static Value v() { return Value::ArrayUShort; }
};
template <>
struct BuilderType_to_Enum_Value<ArrayIntBuilder> : public Enum_Value_Base {
  static Value v() { return Value::ArrayInt; }
};
template <>
struct BuilderType_to_Enum_Value<ArrayUIntBuilder> : public Enum_Value_Base {
  static Value v() { return Value::ArrayUInt; }
};
template <>
struct BuilderType_to_Enum_Value<ArrayLongBuilder> : public Enum_Value_Base {
  static Value v() { return Value::ArrayLong; }
};
template <>
struct BuilderType_to_Enum_Value<ArrayULongBuilder> : public Enum_Value_Base {
  static Value v() { return Value::ArrayULong; }
};
template <>
struct BuilderType_to_Enum_Value<ArrayFloatBuilder> : public Enum_Value_Base {
  static Value v() { return Value::ArrayFloat; }
};
template <>
struct BuilderType_to_Enum_Value<ArrayDoubleBuilder> : public Enum_Value_Base {
  static Value v() { return Value::ArrayDouble; }
};

template <typename T0> class Make_Scalar {
public:
  using T1 = typename std::conditional<
      std::is_same<T0, epics::pvData::boolean>::value, ByteBuilder,
      typename std::conditional<
          std::is_same<T0, int8_t>::value, ByteBuilder,
          typename std::conditional<
              std::is_same<T0, int16_t>::value, ShortBuilder,
              typename std::conditional<
                  std::is_same<T0, int32_t>::value, IntBuilder,
                  typename std::conditional<
                      std::is_same<T0, int64_t>::value, LongBuilder,
                      typename std::conditional<
                          std::is_same<T0, uint8_t>::value, UByteBuilder,
                          typename std::conditional<
                              std::is_same<T0, uint16_t>::value, UShortBuilder,
                              typename std::conditional<
                                  std::is_same<T0, uint32_t>::value,
                                  UIntBuilder,
                                  typename std::conditional<
                                      std::is_same<T0, uint64_t>::value,
                                      ULongBuilder,
                                      typename std::conditional<
                                          std::is_same<T0, float>::value,
                                          FloatBuilder,
                                          typename std::conditional<
                                              std::is_same<T0, double>::value,
                                              DoubleBuilder, std::nullptr_t>::
                                              type>::type>::type>::type>::
                              type>::type>::type>::type>::type>::type>::type;

  static Value_t convert(flatbuffers::FlatBufferBuilder *builder,
                         epics::pvData::PVScalar *field_) {
    auto field = static_cast<epics::pvData::PVScalarValue<T0> *>(field_);
    T1 pv_builder(*builder);
    T0 val = field->get();
    pv_builder.add_value(val);
    return {BuilderType_to_Enum_Value<T1>::v(), pv_builder.Finish().Union()};
  }
};

template <typename T0> class Make_ScalarArray {
public:
  using T1 = typename std::conditional<
      std::is_same<T0, epics::pvData::boolean>::value, ArrayByteBuilder,
      typename std::conditional<
          std::is_same<T0, int8_t>::value, ArrayByteBuilder,
          typename std::conditional<
              std::is_same<T0, int16_t>::value, ArrayShortBuilder,
              typename std::conditional<
                  std::is_same<T0, int32_t>::value, ArrayIntBuilder,
                  typename std::conditional<
                      std::is_same<T0, int64_t>::value, ArrayLongBuilder,
                      typename std::conditional<
                          std::is_same<T0, uint8_t>::value, ArrayUByteBuilder,
                          typename std::conditional<
                              std::is_same<T0, uint16_t>::value,
                              ArrayUShortBuilder,
                              typename std::conditional<
                                  std::is_same<T0, uint32_t>::value,
                                  ArrayUIntBuilder,
                                  typename std::conditional<
                                      std::is_same<T0, uint64_t>::value,
                                      ArrayULongBuilder,
                                      typename std::conditional<
                                          std::is_same<T0, float>::value,
                                          ArrayFloatBuilder,
                                          typename std::conditional<
                                              std::is_same<T0, double>::value,
                                              ArrayDoubleBuilder,
                                              std::nullptr_t>::type>::type>::
                                      type>::type>::type>::type>::type>::type>::
              type>::type>::type;

  using T3 =
      typename std::conditional<std::is_same<T0, epics::pvData::boolean>::value,
                                signed char, T0>::type;

  static Value_t convert(flatbuffers::FlatBufferBuilder *builder,
                         epics::pvData::PVScalarArray *field_, uint8_t opts) {
    auto field = static_cast<epics::pvData::PVValueArray<T0> *>(field_);
    field->setImmutable();
    auto svec = field->view();
    auto nlen = svec.size();

    flatbuffers::Offset<flatbuffers::Vector<T3>> val;
    if (opts == 1) {
      T0 *p1 = nullptr;
      val =
          builder->CreateUninitializedVector(nlen, sizeof(T0), (uint8_t **)&p1);
      memcpy(p1, svec.data(), nlen * sizeof(T0));
    } else {
      val = builder->CreateVector((T3 *)svec.data(), nlen);
    }

    T1 pv_builder(*builder);
    pv_builder.add_value(val);
    return {BuilderType_to_Enum_Value<T1>::v(), pv_builder.Finish().Union()};
  }
};

} // end namespace PVStructureToFlatBufferN

Value_t make_Value_scalar(flatbuffers::FlatBufferBuilder &builder,
                          epics::pvData::PVScalar *field,
                          Statistics &statistics) {
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
    ++statistics.err_not_implemented_yet;
    break;
  }
  return {Value::NONE, 0};
}

Value_t make_Value_array(flatbuffers::FlatBufferBuilder &builder,
                         epics::pvData::PVScalarArray *field, uint8_t opts,
                         Statistics &statistics) {
  using S = epics::pvData::ScalarType;
  using namespace epics::pvData;
  using namespace PVStructureToFlatBufferN;
  switch (field->getScalarArray()->getElementType()) {
  case S::pvBoolean:
    return Make_ScalarArray<epics::pvData::boolean>::convert(&builder, field,
                                                             opts);
  case S::pvByte:
    return Make_ScalarArray<int8_t>::convert(&builder, field, opts);
  case S::pvShort:
    return Make_ScalarArray<int16_t>::convert(&builder, field, opts);
  case S::pvInt:
    return Make_ScalarArray<int32_t>::convert(&builder, field, opts);
  case S::pvLong:
    return Make_ScalarArray<int64_t>::convert(&builder, field, opts);
  case S::pvUByte:
    return Make_ScalarArray<uint8_t>::convert(&builder, field, opts);
  case S::pvUShort:
    return Make_ScalarArray<uint16_t>::convert(&builder, field, opts);
  case S::pvUInt:
    return Make_ScalarArray<uint32_t>::convert(&builder, field, opts);
  case S::pvULong:
    return Make_ScalarArray<uint64_t>::convert(&builder, field, opts);
  case S::pvFloat:
    return Make_ScalarArray<float>::convert(&builder, field, opts);
  case S::pvDouble:
    return Make_ScalarArray<double>::convert(&builder, field, opts);
  case S::pvString:
    ++statistics.err_not_implemented_yet;
    break;
  }
  return {Value::NONE, 0};
}

template <typename T> class release_deleter {
public:
  release_deleter() : do_delete(true) {}
  void operator()(T *ptr) {
    if (do_delete)
      delete ptr;
  }
  bool do_delete;
};

Value_t make_Value(flatbuffers::FlatBufferBuilder &builder,
                   epics::pvData::PVStructurePtr const &field_full,
                   uint8_t opts, Statistics &statistics) {
  if (!field_full) {
    return {Value::NONE, 0};
  }
  auto field = field_full->getSubField("value");
  if (!field) {
    return {Value::NONE, 0};
  }
  // Check the type of 'value'
  // Optionally, compare with name of the PV?
  // Create appropriate fb union
  // CreateVector using the correct types.
  auto t1 = field->getField()->getType();
  using T = epics::pvData::Type;
  switch (t1) {
  case T::scalar:
    return make_Value_scalar(
        builder, static_cast<epics::pvData::PVScalar *>(field.get()),
        statistics);
  case T::scalarArray:
    return make_Value_array(
        builder, static_cast<epics::pvData::PVScalarArray *>(field.get()), opts,
        statistics);
  case T::structure: {
    // supported so far:
    // NTEnum:  we currently send the index value.  full enum identifier is
    // coming when it
    // is decided how we store on nexus side.
    release_deleter<epics::pvData::PVStructure> del;
    del.do_delete = false;
    epics::pvData::PVStructurePtr p1(
        (epics::pvData::PVStructure *)field_full.get(), del);
    if (epics::nt::NTEnum::isCompatible(p1)) {
      auto findex =
          ((epics::pvData::PVStructure *)(field.get()))->getSubField("index");
      return make_Value_scalar(
          builder, static_cast<epics::pvData::PVScalar *>(findex.get()),
          statistics);
      break;
    }
    break;
  }
  case T::structureArray:
    break;
  case T::union_:
    break;
  case T::unionArray:
    break;
  }
  return {Value::NONE, 0};
}

class Converter : public MakeFlatBufferFromPVStructure {
public:
  Converter() {
#ifdef TRACK_SEQ_DATA
    LOG(3, "Converter() with TRACK_SEQ_DATA");
#endif
  }

  ~Converter() override { LOG(3, "~Converter"); }

  BrightnESS::FlatBufs::FB_uptr convert(EpicsPVUpdate const &up) override {
    auto &pvstr = up.epics_pvstr;
    auto fb = BrightnESS::FlatBufs::FB_uptr(new BrightnESS::FlatBufs::FB);

    auto builder = fb->builder.get();
    // this is the field type ID string: up.pvstr->getStructure()->getID()
    auto n = builder->CreateString(up.channel);
    auto vF = make_Value(*builder, pvstr, 1, statistics);

    flatbuffers::Offset<void> fwdinfo = 0;
    if (true) {
      // Was only interesting for forwarder testing
      fwdinfo_1_tBuilder bf(*builder);
      fb->seq = up.seq_fwd;
      fb->fwdix = up.fwdix;
      uint64_t seq_data = 0;
      if (auto x = pvstr->getSubField<epics::pvData::PVScalarValue<uint64_t>>(
              "seq")) {
        seq_data = x->get();
      }
      uint64_t ts_data = 0;
      if (auto x = pvstr->getSubField<epics::pvData::PVScalarValue<uint64_t>>(
              "ts")) {
        ts_data = x->get();
      }
      bf.add_seq_data(seq_data);
      bf.add_seq_fwd(up.seq_fwd);
      bf.add_ts_data(ts_data);
      bf.add_ts_fwd(up.ts_epics_monitor);
      bf.add_fwdix(up.fwdix);
      bf.add_teamid(up.teamid);
      fwdinfo = bf.Finish().Union();
#ifdef TRACK_SEQ_DATA
      seqs.insert(seq_data);
#endif
    }

    LogDataBuilder b(*builder);
    b.add_source_name(n);
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
    } else {
      ++statistics.err_timestamp_not_available;
    }

    b.add_fwdinfo_type(forwarder_internal::fwdinfo_1_t);
    b.add_fwdinfo(fwdinfo);

    FinishLogDataBuffer(*builder, b.Finish());
    if (log_level >= 9) {
      auto b1 = binary_to_hex((char const *)builder->GetBufferPointer(),
                              builder->GetSize());
      uint64_t seq_data = 0;
      flatbuffers::Verifier veri(builder->GetBufferPointer(),
                                 builder->GetSize());
      if (!VerifyLogDataBuffer(veri)) {
        // TODO
        // Handle this situation more gracefully...
        throw std::runtime_error("Can not verify the just cretaed buffer");
      }
      auto d1 = GetLogData(builder->GetBufferPointer());
      if (auto fi = d1->fwdinfo()) {
        if (d1->fwdinfo_type() == forwarder_internal::fwdinfo_1_t) {
          seq_data = static_cast<fwdinfo_1_t const *>(fi)->seq_data();
        }
      }
      LOG(9, "seq data/fwd: {} / {}  schema: [{}]\n{:.{}}", seq_data,
          up.seq_fwd, LogDataIdentifier(), b1.data(), b1.size());
    }
    return fb;
  }

  std::map<std::string, double> stats() override {
    return {{"ranges_n", seqs.size()}};
  }

  RangeSet<uint64_t> seqs;
  Statistics statistics;
};

/// This class is purely for testing
class ConverterTestNamed : public MakeFlatBufferFromPVStructure {
public:
  BrightnESS::FlatBufs::FB_uptr convert(EpicsPVUpdate const &up) override {
    auto &pvstr = up.epics_pvstr;

    {
      // epics madness
      auto f1 = pvstr->getSubField<epics::pvData::PVField>("value");
      if (f1) {
        auto f2 = f1->getField();
        auto ft = f2->getType();
        if (ft == epics::pvData::Type::scalarArray) {
          auto f3 = pvstr->getSubField<epics::pvData::PVScalarArray>("value");
          auto f4 = f3->getScalarArray();
          if (f4->getElementType() == epics::pvData::ScalarType::pvInt) {
            had_int32 += 1;
          }
          if (f4->getElementType() == epics::pvData::ScalarType::pvDouble) {
            had_double += 1;
          }
        }
      }
    }

    auto fb = BrightnESS::FlatBufs::FB_uptr(new BrightnESS::FlatBufs::FB);
    auto builder = fb->builder.get();
    using uchar = unsigned char;
    static_assert(sizeof(uchar) == 1, "");
    {
      uint32_t *p1 = nullptr;
      auto fba = builder->CreateUninitializedVector(2, 4, (uint8_t **)&p1);
      p1[0] = had_int32.load();
      p1[1] = had_double.load();
      ArrayUIntBuilder b2(*builder);
      b2.add_value(fba);
      auto fbval = b2.Finish().Union();
      auto n = builder->CreateString(up.channel);
      LogDataBuilder b(*builder);
      b.add_source_name(n);
      b.add_value_type(Value::ArrayUInt);
      b.add_value(fbval);
      FinishLogDataBuffer(*builder, b.Finish());
      return fb;
    }
  }

  std::atomic<uint32_t> had_int32{0};
  std::atomic<uint32_t> had_double{0};
};

class Info : public SchemaInfo {
public:
  MakeFlatBufferFromPVStructure::ptr create_converter() override;
};

MakeFlatBufferFromPVStructure::ptr Info::create_converter() {
  return MakeFlatBufferFromPVStructure::ptr(new Converter);
}

class InfoNamedConverter : public SchemaInfo {
public:
  MakeFlatBufferFromPVStructure::ptr create_converter() override;
};

MakeFlatBufferFromPVStructure::ptr InfoNamedConverter::create_converter() {
  return MakeFlatBufferFromPVStructure::ptr(new ConverterTestNamed);
}

FlatBufs::SchemaRegistry::Registrar<Info> g_registrar_info("f142",
                                                           Info::ptr(new Info));
FlatBufs::SchemaRegistry::Registrar<Info>
    g_registrar_info_test_named_converter("f142-test-named-converter",
                                          Info::ptr(new InfoNamedConverter));
} // namespace f142
} // namespace FlatBufs
} // namespace BrightnESS
