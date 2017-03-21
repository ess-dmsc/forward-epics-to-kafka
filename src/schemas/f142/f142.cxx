#include "../../logger.h"
#include "../../SchemaRegistry.h"
#include "../../helper.h"
#include "../../epics-to-fb.h"
#include "schemas/f142_logdata_generated.h"

namespace BrightnESS {
namespace FlatBufs {
namespace f142 {

#define STRINGIFY2(x) #x
#define STRINGIFY(x) STRINGIFY2(x)


template <typename T> char const * type_name();
//template <> char const * type_name<uint32_t>() { return "uint32_t"; }
// Unstringification not possible, so not possible to give white space..
#define M(x) template <> char const * type_name<x>() { return STRINGIFY(x); }
M( int8_t)
M(uint8_t)
M( int16_t)
M(uint16_t)
M( int32_t)
M(uint32_t)
M( int64_t)
M(uint64_t)
M(float)
M(double)
#undef M


typedef struct { Value type; flatbuffers::Offset<void> off; } Value_t;


namespace PVStructureToFlatBufferN {

struct Enum_Value_Base { };

template <typename T0> struct BuilderType_to_Enum_Value : public Enum_Value_Base { static Value v(); };
template <> struct BuilderType_to_Enum_Value<ByteBuilder>        : public Enum_Value_Base { static Value v() { return Value::Byte; } };
template <> struct BuilderType_to_Enum_Value<UByteBuilder>       : public Enum_Value_Base { static Value v() { return Value::UByte; } };
template <> struct BuilderType_to_Enum_Value<ShortBuilder>       : public Enum_Value_Base { static Value v() { return Value::Short; } };
template <> struct BuilderType_to_Enum_Value<UShortBuilder>      : public Enum_Value_Base { static Value v() { return Value::UShort; } };
template <> struct BuilderType_to_Enum_Value<IntBuilder>         : public Enum_Value_Base { static Value v() { return Value::Int; } };
template <> struct BuilderType_to_Enum_Value<UIntBuilder>        : public Enum_Value_Base { static Value v() { return Value::UInt; } };
template <> struct BuilderType_to_Enum_Value<LongBuilder>        : public Enum_Value_Base { static Value v() { return Value::Long; } };
template <> struct BuilderType_to_Enum_Value<ULongBuilder>       : public Enum_Value_Base { static Value v() { return Value::ULong; } };
template <> struct BuilderType_to_Enum_Value<FloatBuilder>       : public Enum_Value_Base { static Value v() { return Value::Float; } };
template <> struct BuilderType_to_Enum_Value<DoubleBuilder>      : public Enum_Value_Base { static Value v() { return Value::Double; } };

template <> struct BuilderType_to_Enum_Value<ArrayByteBuilder>   : public Enum_Value_Base { static Value v() { return Value::ArrayByte; } };
template <> struct BuilderType_to_Enum_Value<ArrayUByteBuilder>  : public Enum_Value_Base { static Value v() { return Value::ArrayUByte; } };
template <> struct BuilderType_to_Enum_Value<ArrayShortBuilder>  : public Enum_Value_Base { static Value v() { return Value::ArrayShort; } };
template <> struct BuilderType_to_Enum_Value<ArrayUShortBuilder> : public Enum_Value_Base { static Value v() { return Value::ArrayUShort; } };
template <> struct BuilderType_to_Enum_Value<ArrayIntBuilder>    : public Enum_Value_Base { static Value v() { return Value::ArrayInt; } };
template <> struct BuilderType_to_Enum_Value<ArrayUIntBuilder>   : public Enum_Value_Base { static Value v() { return Value::ArrayUInt; } };
template <> struct BuilderType_to_Enum_Value<ArrayLongBuilder>   : public Enum_Value_Base { static Value v() { return Value::ArrayLong; } };
template <> struct BuilderType_to_Enum_Value<ArrayULongBuilder>  : public Enum_Value_Base { static Value v() { return Value::ArrayULong; } };
template <> struct BuilderType_to_Enum_Value<ArrayFloatBuilder>  : public Enum_Value_Base { static Value v() { return Value::ArrayFloat; } };
template <> struct BuilderType_to_Enum_Value<ArrayDoubleBuilder> : public Enum_Value_Base { static Value v() { return Value::ArrayDouble; } };





template <typename T0>
class Make_Scalar {
public:
using T1 = typename std::conditional<
	std::is_same<T0, epics::pvData::boolean >::value, ByteBuilder, typename std::conditional<
	std::is_same<T0,    int8_t   >::value, ByteBuilder,   typename std::conditional<
	std::is_same<T0,    int16_t  >::value, ShortBuilder,  typename std::conditional<
	std::is_same<T0,    int32_t  >::value, IntBuilder,    typename std::conditional<
	std::is_same<T0,    int64_t  >::value, LongBuilder,   typename std::conditional<
	std::is_same<T0,   uint8_t   >::value, UByteBuilder,  typename std::conditional<
	std::is_same<T0,   uint16_t  >::value, UShortBuilder, typename std::conditional<
	std::is_same<T0,   uint32_t  >::value, UIntBuilder,   typename std::conditional<
	std::is_same<T0,   uint64_t  >::value, ULongBuilder,  typename std::conditional<
	std::is_same<T0,   float     >::value, FloatBuilder,  typename std::conditional<
	std::is_same<T0,   double    >::value, DoubleBuilder, std::nullptr_t
	>::type
	>::type
	>::type
	>::type
	>::type
	>::type
	>::type
	>::type
	>::type
	>::type
	>::type;

static Value_t convert(flatbuffers::FlatBufferBuilder * builder, epics::pvData::PVScalar * field_) {
	auto field = static_cast<epics::pvData::PVScalarValue<T0>*>(field_);
	T1 pv_builder(*builder);
	T0 val = field->get();
	pv_builder.add_value(val);
	return {BuilderType_to_Enum_Value<T1>::v(), pv_builder.Finish().Union()};
}

};




template <typename T0>
class Make_ScalarArray {
public:
using T1 = typename std::conditional<
	std::is_same<T0, epics::pvData::boolean>::value, ArrayByteBuilder, typename std::conditional<
	std::is_same<T0,  int8_t >::value, ArrayByteBuilder,    typename std::conditional<
	std::is_same<T0,  int16_t>::value, ArrayShortBuilder,   typename std::conditional<
	std::is_same<T0,  int32_t>::value, ArrayIntBuilder,     typename std::conditional<
	std::is_same<T0,  int64_t>::value, ArrayLongBuilder,    typename std::conditional<
	std::is_same<T0, uint8_t >::value, ArrayUByteBuilder,   typename std::conditional<
	std::is_same<T0, uint16_t>::value, ArrayUShortBuilder,  typename std::conditional<
	std::is_same<T0, uint32_t>::value, ArrayUIntBuilder,    typename std::conditional<
	std::is_same<T0, uint64_t>::value, ArrayULongBuilder,   typename std::conditional<
	std::is_same<T0, float   >::value, ArrayFloatBuilder,   typename std::conditional<
	std::is_same<T0, double  >::value, ArrayDoubleBuilder,  std::nullptr_t
	>::type
	>::type
	>::type
	>::type
	>::type
	>::type
	>::type
	>::type
	>::type
	>::type
	>::type;

using T3 = typename std::conditional< std::is_same<T0, epics::pvData::boolean>::value, signed char, T0 >::type;

static Value_t convert(flatbuffers::FlatBufferBuilder * builder, epics::pvData::PVScalarArray * field_) {
	auto field = static_cast<epics::pvData::PVValueArray<T0>*>(field_);
	auto svec = field->view();
	auto nlen = svec.size();
	auto val = builder->CreateVector((T3*)svec.data(), nlen);
	T1 pv_builder(*builder);
	pv_builder.add_value(val);
	return {BuilderType_to_Enum_Value<T1>::v(), pv_builder.Finish().Union()};
}

};


} // end namespace PVStructureToFlatBufferN




Value_t make_Value_scalar(flatbuffers::FlatBufferBuilder & builder, epics::pvData::PVScalar * field) {
	using S = epics::pvData::ScalarType;
	using namespace epics::pvData;
	using namespace PVStructureToFlatBufferN;
	switch (field->getScalar()->getScalarType()) {
	case S::pvBoolean:
		return Make_Scalar< epics::pvData::boolean >::convert(&builder, field);
	case S::pvByte:
		return Make_Scalar<  int8_t >::convert(&builder, field);
	case S::pvShort:
		return Make_Scalar<  int16_t >::convert(&builder, field);
	case S::pvInt:
		return Make_Scalar<  int32_t >::convert(&builder, field);
	case S::pvLong:
		return Make_Scalar<  int64_t >::convert(&builder, field);
	case S::pvUByte:
		return Make_Scalar< uint8_t >::convert(&builder, field);
	case S::pvUShort:
		return Make_Scalar< uint16_t >::convert(&builder, field);
	case S::pvUInt:
		return Make_Scalar< uint32_t >::convert(&builder, field);
	case S::pvULong:
		return Make_Scalar< uint64_t >::convert(&builder, field);
	case S::pvFloat:
		return Make_Scalar< float >::convert(&builder, field);
	case S::pvDouble:
		return Make_Scalar< double >::convert(&builder, field);
	case S::pvString:
		// Sorry, not implemented yet
		LOG(5, "ERROR pvString not implemented yet");
		break;
	}
	return {Value::Byte, 0};
}

Value_t make_Value_array(flatbuffers::FlatBufferBuilder & builder, epics::pvData::PVScalarArray * field) {
	using S = epics::pvData::ScalarType;
	using namespace epics::pvData;
	using namespace PVStructureToFlatBufferN;
	switch (field->getScalarArray()->getElementType()) {
	case S::pvBoolean:
		return Make_ScalarArray< epics::pvData::boolean >::convert(&builder, field);
	case S::pvByte:
		return Make_ScalarArray<  int8_t >::convert(&builder, field);
	case S::pvShort:
		return Make_ScalarArray<  int16_t >::convert(&builder, field);
	case S::pvInt:
		return Make_ScalarArray<  int32_t >::convert(&builder, field);
	case S::pvLong:
		return Make_ScalarArray<  int64_t >::convert(&builder, field);
	case S::pvUByte:
		return Make_ScalarArray< uint8_t >::convert(&builder, field);
	case S::pvUShort:
		return Make_ScalarArray< uint16_t >::convert(&builder, field);
	case S::pvUInt:
		return Make_ScalarArray< uint32_t >::convert(&builder, field);
	case S::pvULong:
		return Make_ScalarArray< uint64_t >::convert(&builder, field);
	case S::pvFloat:
		return Make_ScalarArray< float >::convert(&builder, field);
	case S::pvDouble:
		return Make_ScalarArray< double >::convert(&builder, field);
	case S::pvString:
		// Sorry, not implemented yet
		LOG(5, "ERROR pvString not implemented yet");
		break;
	}
	return {Value::Byte, 0};
}

Value_t make_Value(flatbuffers::FlatBufferBuilder & builder, epics::pvData::PVFieldPtr const & field) {
	if (!field) {
		LOG(2, "can not do anything with a null pointer");
		return {Value::Byte, 0};
	}
	// Check the type of 'value'
	// Optionally, compare with name of the PV?
	// Create appropriate fb union
	// CreateVector using the correct types.
	auto t1 = field->getField()->getType();
	using T = epics::pvData::Type;
	switch (t1) {
	case T::scalar:
		return make_Value_scalar(builder, static_cast< epics::pvData::PVScalar*>(field.get()));
	case T::scalarArray:
		return make_Value_array(builder, static_cast< epics::pvData::PVScalarArray*>(field.get()));
	case T::structure:
		LOG(5, "Type::structure can not be handled");
		break;
	case T::structureArray:
		LOG(5, "Type::structureArray can not be handled");
		break;
	case T::union_:
		LOG(5, "Type::union_ can not be handled");
		break;
	case T::unionArray:
		LOG(5, "Type::unionArray can not be handled");
		break;
	}
	return {Value::Byte, 0};
}





class Converter : public MakeFlatBufferFromPVStructure {
public:
BrightnESS::FlatBufs::FB_uptr convert(EpicsPVUpdate const & up) override {
	auto fb = BrightnESS::FlatBufs::FB_uptr(new BrightnESS::FlatBufs::FB);

	auto builder = fb->builder.get();
	// this is the field type ID string: up.pvstr->getStructure()->getID()
	auto n = builder->CreateString(up.channel);
	auto vF = make_Value(*builder, up.pvstr->getSubField("value"));

	flatbuffers::Offset<void> fwdinfo = 0;
	if (true) {
		// Was only interesting for forwarder testing
		fwdinfo_1_tBuilder bf(*builder);
		fb->seq = up.seq;
		fb->fwdix = up.fwdix;
		uint64_t seq_data = 0;
		if (auto x = up.pvstr->getSubField<epics::pvData::PVScalarValue<uint64_t>>("seq")) {
			seq_data = x->get();
		}
		uint64_t ts_data = 0;
		if (auto x = up.pvstr->getSubField<epics::pvData::PVScalarValue<uint64_t>>("ts")) {
			ts_data = x->get();
		}
		bf.add_seq_data(seq_data);
		bf.add_seq_fwd(up.seq);
		bf.add_ts_data(ts_data);
		bf.add_ts_fwd(up.ts_epics_monitor);
		bf.add_fwdix(up.fwdix);
		bf.add_teamid(up.teamid);
		fwdinfo = bf.Finish().Union();
	}

	LogDataBuilder b(*builder);
	b.add_source_name(n);
	b.add_value_type(vF.type);
	b.add_value(vF.off);

	if (auto pvTimeStamp = up.pvstr->getSubField<epics::pvData::PVStructure>("timeStamp")) {
		uint64_t ts = (uint64_t)pvTimeStamp->getSubField<epics::pvData::PVScalarValue<int64_t>>("secondsPastEpoch")->get();
		ts *= 1000000000;
		ts += pvTimeStamp->getSubField<epics::pvData::PVScalarValue<int32_t>>("nanoseconds")->get();
		b.add_timestamp(ts);
	}
	else {
		LOG(5, "timeStamp on PV not available");
	}

	b.add_fwdinfo_type(forwarder_internal::fwdinfo_1_t);
	b.add_fwdinfo(fwdinfo);

	FinishLogDataBuffer(*builder, b.Finish());
	if (log_level >= 9) {
		auto b1 = binary_to_hex((char const *)builder->GetBufferPointer(), builder->GetSize());
		uint64_t seq_data = 0;
		flatbuffers::Verifier veri(builder->GetBufferPointer(), builder->GetSize());
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
		LOG(9, "seq data/fwd: {} / {}  schema: [{}]\n{:.{}}", seq_data, up.seq, LogDataIdentifier(), b1.data(), b1.size());
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


FlatBufs::SchemaRegistry::Registrar<Info> g_registrar_info("f142", Info::ptr(new Info));



}
}
}
