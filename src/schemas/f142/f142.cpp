#include "../../EpicsPVUpdate.h"
#include "../../RangeSet.h"
#include "../../SchemaRegistry.h"
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

namespace FlatBufs {
namespace f142 {

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
struct BuilderType_to_Enum_Value<StringBuilder> : public Enum_Value_Base {
  static Value v() { return Value::String; }
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
  // clang-format off
  using T1 =
    typename std::conditional<std::is_same<T0, epics::pvData::boolean>::value, ByteBuilder,
    typename std::conditional<std::is_same<T0,   int8_t>::value, ByteBuilder,
    typename std::conditional<std::is_same<T0,  int16_t>::value, ShortBuilder,
    typename std::conditional<std::is_same<T0,  int32_t>::value, IntBuilder,
    typename std::conditional<std::is_same<T0,  int64_t>::value, LongBuilder,
    typename std::conditional<std::is_same<T0,  uint8_t>::value, UByteBuilder,
    typename std::conditional<std::is_same<T0, uint16_t>::value, UShortBuilder,
    typename std::conditional<std::is_same<T0, uint32_t>::value, UIntBuilder,
    typename std::conditional<std::is_same<T0, uint64_t>::value, ULongBuilder,
    typename std::conditional<std::is_same<T0,    float>::value, FloatBuilder,
    typename std::conditional<std::is_same<T0,   double>::value, DoubleBuilder,
    std::nullptr_t>::type>::type>::type>::type>::type>::type>::type>::type>::type>::type>::type;
  // clang-format on

  static Value_t convert(flatbuffers::FlatBufferBuilder *Builder,
                         epics::pvData::PVScalar *field_) {
    auto field = static_cast<epics::pvData::PVScalarValue<T0> *>(field_);
    T1 pv_builder(*Builder);
    T0 Value = field->get();
    pv_builder.add_value(Value);
    return {BuilderType_to_Enum_Value<T1>::v(), pv_builder.Finish().Union()};
  }
};

template <typename T0> class Make_ScalarArray {
public:
  // clang-format off
  using T1 =
      typename std::conditional<std::is_same<T0, epics::pvData::boolean>::value, ArrayByteBuilder,
      typename std::conditional<std::is_same<T0, int8_t>::value, ArrayByteBuilder,
      typename std::conditional<std::is_same<T0, int16_t>::value, ArrayShortBuilder,
      typename std::conditional<std::is_same<T0, int32_t>::value, ArrayIntBuilder,
      typename std::conditional<std::is_same<T0, int64_t>::value, ArrayLongBuilder,
      typename std::conditional<std::is_same<T0, uint8_t>::value, ArrayUByteBuilder,
      typename std::conditional<std::is_same<T0, uint16_t>::value, ArrayUShortBuilder,
      typename std::conditional<std::is_same<T0, uint32_t>::value, ArrayUIntBuilder,
      typename std::conditional<std::is_same<T0, uint64_t>::value, ArrayULongBuilder,
      typename std::conditional<std::is_same<T0, float>::value,ArrayFloatBuilder,
      typename std::conditional<std::is_same<T0, double>::value, ArrayDoubleBuilder,
      std::nullptr_t>::type>::type>::type>::type>::type>::type>::type>::type>::type>::type>::type;
  // clang-format on

  using T3 =
      typename std::conditional<std::is_same<T0, epics::pvData::boolean>::value,
                                signed char, T0>::type;

  static Value_t convert(flatbuffers::FlatBufferBuilder *Builder,
                         epics::pvData::PVScalarArray *field_, bool opts) {
    auto field = static_cast<epics::pvData::PVValueArray<T0> *>(field_);
    field->setImmutable();
    auto svec = field->view();
    auto nlen = svec.size();

    flatbuffers::Offset<flatbuffers::Vector<T3>> val;
    if (opts) {
      T0 *p1 = nullptr;
      val =
          Builder->CreateUninitializedVector(nlen, sizeof(T0), (uint8_t **)&p1);
      memcpy(p1, svec.data(), nlen * sizeof(T0));
    } else {
      val = Builder->CreateVector((T3 *)svec.data(), nlen);
    }

    T1 pv_builder(*Builder);
    pv_builder.add_value(val);
    return {BuilderType_to_Enum_Value<T1>::v(), pv_builder.Finish().Union()};
  }
};

class MakeScalarString {
public:
  static Value_t convert(flatbuffers::FlatBufferBuilder *Builder,
                         epics::pvData::PVScalar *PVScalarValue) {
    auto PVScalarString =
        dynamic_cast<epics::pvData::PVScalarValue<std::string> *>(
            PVScalarValue);
    std::string Value = PVScalarString->get();
    auto FlatbufferedValueString =
        Builder->CreateString(Value.data(), Value.size());
    StringBuilder ValueBuilder(*Builder);
    ValueBuilder.add_value(FlatbufferedValueString);
    return {BuilderType_to_Enum_Value<StringBuilder>::v(),
            ValueBuilder.Finish().Union()};
  }
};

} // end namespace PVStructureToFlatBufferN

Value_t make_Value_scalar(flatbuffers::FlatBufferBuilder &builder,
                          epics::pvData::PVScalar *field, Statistics &Stats) {
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
    return MakeScalarString::convert(&builder, field);
  default:
    ++Stats.err_not_implemented_yet;
    break;
  }
  return {Value::NONE, 0};
}

Value_t make_Value_array(flatbuffers::FlatBufferBuilder &builder,
                         epics::pvData::PVScalarArray *field, bool opts,
                         Statistics &Stats) {
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
    ++Stats.err_not_implemented_yet;
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

Value_t make_Value(flatbuffers::FlatBufferBuilder &Builder,
                   epics::pvData::PVStructurePtr const &PVStructureField,
                   bool opts, Statistics &statistics) {
  if (!PVStructureField) {
    return {Value::NONE, 0};
  }
  auto ValueField = PVStructureField->getSubField("value");
  if (!ValueField) {
    return {Value::NONE, 0};
  }
  // Check the type of 'value'
  // Optionally, compare with name of the PV?
  // Create appropriate fb union
  // CreateVector using the correct types.
  auto ValueType = ValueField->getField()->getType();
  using PVType = epics::pvData::Type;
  switch (ValueType) {
  case PVType::scalar:
    return make_Value_scalar(
        Builder, dynamic_cast<epics::pvData::PVScalar *>(ValueField.get()),
        statistics);
  case PVType::scalarArray:
    return make_Value_array(
        Builder, dynamic_cast<epics::pvData::PVScalarArray *>(ValueField.get()),
        opts, statistics);
  case PVType::structure: {
    // supported so far:
    // NTEnum:  we currently send the index value.  full enum identifier is
    // coming when it
    // is decided how we store on nexus side.
    release_deleter<epics::pvData::PVStructure> del;
    del.do_delete = false;
    epics::pvData::PVStructurePtr p1(PVStructureField.get(), del);
    if (epics::nt::NTEnum::isCompatible(p1)) {
      auto findex = ((epics::pvData::PVStructure *)(ValueField.get()))
                        ->getSubField("index");
      return make_Value_scalar(
          Builder, dynamic_cast<epics::pvData::PVScalar *>(findex.get()),
          statistics);
    }
    break;
  }
  case PVType::structureArray:
    break;
  case PVType::union_:
    break;
  case PVType::unionArray:
    break;
  }
  return {Value::NONE, 0};
}

class Converter : public FlatBufferCreator {
public:
  Converter() = default;

  ~Converter() override { LOG(Sev::Error, "~Converter"); }

  std::unique_ptr<FlatBufs::FlatbufferMessage>
  create(EpicsPVUpdate const &PVUpdate) override {
    auto &PVStructure = PVUpdate.epics_pvstr;
    auto FlatbufferMessage = make_unique<FlatBufs::FlatbufferMessage>();

    auto Builder = FlatbufferMessage->builder.get();
    // this is the field type ID string: up.pvstr->getStructure()->getID()
    auto PVName = Builder->CreateString(PVUpdate.channel);
    auto Value = make_Value(*Builder, PVStructure, true, Stats);

    LogDataBuilder LogDataBuilder(*Builder);
    LogDataBuilder.add_source_name(PVName);
    LogDataBuilder.add_value_type(Value.type);
    LogDataBuilder.add_value(Value.off);

    if (auto PVTimeStamp =
            PVStructure->getSubField<epics::pvData::PVStructure>("timeStamp")) {
      uint64_t TimeStamp =
          (uint64_t)PVTimeStamp
              ->getSubField<epics::pvData::PVScalarValue<int64_t>>(
                  "secondsPastEpoch")
              ->get();
      TimeStamp *= 1000000000;
      TimeStamp += PVTimeStamp
                       ->getSubField<epics::pvData::PVScalarValue<int32_t>>(
                           "nanoseconds")
                       ->get();
      LogDataBuilder.add_timestamp(TimeStamp);
    } else {
      ++Stats.err_timestamp_not_available;
    }

    FinishLogDataBuffer(*Builder, LogDataBuilder.Finish());
    return FlatbufferMessage;
  }

  std::map<std::string, double> getStats() override {
    return {{"ranges_n", seqs.size()}};
  }

  RangeSet<uint64_t> seqs;
  Statistics Stats;
};

/// This class is purely for testing
class ConverterTestNamed : public FlatBufferCreator {
public:
  std::unique_ptr<FlatBufs::FlatbufferMessage>
  create(EpicsPVUpdate const &up) override {
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

    auto fb = make_unique<FlatBufs::FlatbufferMessage>();
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
  std::unique_ptr<FlatBufferCreator> create_converter() override;
};

std::unique_ptr<FlatBufferCreator> Info::create_converter() {
  return make_unique<Converter>();
}

class InfoNamedConverter : public SchemaInfo {
public:
  std::unique_ptr<FlatBufferCreator> create_converter() override;
};

std::unique_ptr<FlatBufferCreator> InfoNamedConverter::create_converter() {
  return make_unique<ConverterTestNamed>();
}

FlatBufs::SchemaRegistry::Registrar<Info> g_registrar_info("f142",
                                                           Info::ptr(new Info));
FlatBufs::SchemaRegistry::Registrar<Info>
    g_registrar_info_test_named_converter("f142-test-named-converter",
                                          Info::ptr(new InfoNamedConverter));
} // namespace f142
} // namespace FlatBufs