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




class PVStructureToFlatBuffer {
public:
using FBT = BrightnESS::FlatBufs::FB_uptr;
using ptr = std::unique_ptr<PVStructureToFlatBuffer>;
static ptr create(epics::pvData::PVStructure::shared_pointer & pvstr);
virtual FBT convert(uint64_t seq, uint64_t ts, TopicMappingSettings & settings, epics::pvData::PVStructure::shared_pointer & pvstr) = 0;
};

namespace PVStructureToFlatBufferN {


void add_name_timeStamp(flatbuffers::FlatBufferBuilder & b1, EpicsPVBuilder & b2, std::string & channel_name, epics::pvData::PVStructure::shared_pointer & pvstr) {
	auto pvTimeStamp = pvstr->getSubField<epics::pvData::PVStructure>("timeStamp");
	if (not pvTimeStamp) {
		LOG(0, "timeStamp not available");
		return;
	}
	timeStamp_t timeStamp(
		pvTimeStamp->getSubField<epics::pvData::PVScalarValue<int64_t>>("secondsPastEpoch")->get(),
		pvTimeStamp->getSubField<epics::pvData::PVScalarValue<int>>("nanoseconds")->get()
	);
	//LOG(5, "secondsPastEpoch: {:20}", timeStamp.secondsPastEpoch());
	//LOG(5, "nanoseconds:      {:20}", timeStamp.nanoseconds());
	b2.add_timeStamp(&timeStamp);
}



struct Enum_PV_Base {
using RET = BrightnESS::ForwardEpicsToKafka::Epics::PV;
};

template <typename T0> struct BuilderType_to_Enum_PV : public Enum_PV_Base { static RET v(); };
template <> struct BuilderType_to_Enum_PV<NTScalarByteBuilder>        : public Enum_PV_Base { static RET v() { return PV::NTScalarByte; } };
template <> struct BuilderType_to_Enum_PV<NTScalarUByteBuilder>       : public Enum_PV_Base { static RET v() { return PV::NTScalarUByte; } };
template <> struct BuilderType_to_Enum_PV<NTScalarShortBuilder>       : public Enum_PV_Base { static RET v() { return PV::NTScalarShort; } };
template <> struct BuilderType_to_Enum_PV<NTScalarUShortBuilder>      : public Enum_PV_Base { static RET v() { return PV::NTScalarUShort; } };
template <> struct BuilderType_to_Enum_PV<NTScalarIntBuilder>         : public Enum_PV_Base { static RET v() { return PV::NTScalarInt; } };
template <> struct BuilderType_to_Enum_PV<NTScalarUIntBuilder>        : public Enum_PV_Base { static RET v() { return PV::NTScalarUInt; } };
template <> struct BuilderType_to_Enum_PV<NTScalarLongBuilder>        : public Enum_PV_Base { static RET v() { return PV::NTScalarLong; } };
template <> struct BuilderType_to_Enum_PV<NTScalarULongBuilder>       : public Enum_PV_Base { static RET v() { return PV::NTScalarULong; } };
template <> struct BuilderType_to_Enum_PV<NTScalarFloatBuilder>       : public Enum_PV_Base { static RET v() { return PV::NTScalarFloat; } };
template <> struct BuilderType_to_Enum_PV<NTScalarDoubleBuilder>      : public Enum_PV_Base { static RET v() { return PV::NTScalarDouble; } };

template <> struct BuilderType_to_Enum_PV<NTScalarArrayByteBuilder>   : public Enum_PV_Base { static RET v() { return PV::NTScalarArrayByte; } };
template <> struct BuilderType_to_Enum_PV<NTScalarArrayUByteBuilder>  : public Enum_PV_Base { static RET v() { return PV::NTScalarArrayUByte; } };
template <> struct BuilderType_to_Enum_PV<NTScalarArrayShortBuilder>  : public Enum_PV_Base { static RET v() { return PV::NTScalarArrayShort; } };
template <> struct BuilderType_to_Enum_PV<NTScalarArrayUShortBuilder> : public Enum_PV_Base { static RET v() { return PV::NTScalarArrayUShort; } };
template <> struct BuilderType_to_Enum_PV<NTScalarArrayIntBuilder>    : public Enum_PV_Base { static RET v() { return PV::NTScalarArrayInt; } };
template <> struct BuilderType_to_Enum_PV<NTScalarArrayUIntBuilder>   : public Enum_PV_Base { static RET v() { return PV::NTScalarArrayUInt; } };
template <> struct BuilderType_to_Enum_PV<NTScalarArrayLongBuilder>   : public Enum_PV_Base { static RET v() { return PV::NTScalarArrayLong; } };
template <> struct BuilderType_to_Enum_PV<NTScalarArrayULongBuilder>  : public Enum_PV_Base { static RET v() { return PV::NTScalarArrayULong; } };
template <> struct BuilderType_to_Enum_PV<NTScalarArrayFloatBuilder>  : public Enum_PV_Base { static RET v() { return PV::NTScalarArrayFloat; } };
template <> struct BuilderType_to_Enum_PV<NTScalarArrayDoubleBuilder> : public Enum_PV_Base { static RET v() { return PV::NTScalarArrayDouble; } };





template <typename T0>
class NTScalar : public PVStructureToFlatBuffer {
public:
using T1 = typename std::conditional<
	std::is_same<T0, epics::pvData::boolean >::value, NTScalarByteBuilder, typename std::conditional<
	std::is_same<T0,    int8_t   >::value, NTScalarByteBuilder,   typename std::conditional<
	std::is_same<T0,    int16_t  >::value, NTScalarShortBuilder,  typename std::conditional<
	std::is_same<T0,    int32_t  >::value, NTScalarIntBuilder,    typename std::conditional<
	std::is_same<T0,    int64_t  >::value, NTScalarLongBuilder,   typename std::conditional<
	std::is_same<T0,   uint8_t   >::value, NTScalarUByteBuilder,  typename std::conditional<
	std::is_same<T0,   uint16_t  >::value, NTScalarUShortBuilder, typename std::conditional<
	std::is_same<T0,   uint32_t  >::value, NTScalarUIntBuilder,   typename std::conditional<
	std::is_same<T0,   uint64_t  >::value, NTScalarULongBuilder,  typename std::conditional<
	std::is_same<T0,   float     >::value, NTScalarFloatBuilder,  typename std::conditional<
	std::is_same<T0,   double    >::value, NTScalarDoubleBuilder, std::nullptr_t
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

FBT convert(uint64_t seq, uint64_t ts, TopicMappingSettings & tms, epics::pvData::PVStructure::shared_pointer & pvstr) override {
	FBT fb( new BrightnESS::FlatBufs::FB( FlatBufs::Schema::Simple ) );
	auto builder = fb->builder.get();
	T1 scalar_builder(*builder);
	{
		auto f1 = pvstr->getSubField("value");
		// NOTE
		// We rely on that we inspect the Epics field correctly on first read, and that the
		// Epics server will not silently change type.
		auto f2 = static_cast<epics::pvData::PVScalarValue<T0>*>(f1.get());
		T0 val = f2->get();
		//auto hex = binary_to_hex((char*)&val, sizeof(T0));
		//LOG(1, "packing {}: {:.{}}", typeid(T0).name(), hex.data(), hex.size());
		scalar_builder.add_value(val);
	}
	auto scalar_fin = scalar_builder.Finish();

	// Adding name not moved yet into the add_name_timeStamp, because CreateString would be nested.
	// Therefore, create that string first.
	auto off_name = builder->CreateString(tms.channel);

	EpicsPVBuilder pv_builder(*builder);

	pv_builder.add_name(off_name);

	add_name_timeStamp(*builder, pv_builder, tms.channel, pvstr);
	pv_builder.add_pv_type(BuilderType_to_Enum_PV<T1>::v());
	pv_builder.add_pv(scalar_fin.Union());

	pv_builder.add_seq(seq);
	pv_builder.add_ts(ts);
	pv_builder.add_ts_epics_server(pvstr->getSubField<epics::pvData::PVScalarValue<uint64_t>>("ts")->get());

	builder->Finish(pv_builder.Finish());

	return fb;
}

};



template <typename T0>
class NTScalarArray : public PVStructureToFlatBuffer {
public:
using T1 = typename std::conditional<
	std::is_same<T0, epics::pvData::boolean>::value, NTScalarArrayByteBuilder, typename std::conditional<
	std::is_same<T0,  int8_t >::value, NTScalarArrayByteBuilder,    typename std::conditional<
	std::is_same<T0,  int16_t>::value, NTScalarArrayShortBuilder,   typename std::conditional<
	std::is_same<T0,  int32_t>::value, NTScalarArrayIntBuilder,     typename std::conditional<
	std::is_same<T0,  int64_t>::value, NTScalarArrayLongBuilder,    typename std::conditional<
	std::is_same<T0, uint8_t >::value, NTScalarArrayUByteBuilder,   typename std::conditional<
	std::is_same<T0, uint16_t>::value, NTScalarArrayUShortBuilder,  typename std::conditional<
	std::is_same<T0, uint32_t>::value, NTScalarArrayUIntBuilder,    typename std::conditional<
	std::is_same<T0, uint64_t>::value, NTScalarArrayULongBuilder,   typename std::conditional<
	std::is_same<T0, float   >::value, NTScalarArrayFloatBuilder,   typename std::conditional<
	std::is_same<T0, double  >::value, NTScalarArrayDoubleBuilder,  std::nullptr_t
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

template <typename T> using _O = flatbuffers::Offset<T>;
#define _F BrightnESS::ForwardEpicsToKafka::Epics
using T2 = typename std::conditional<
	std::is_same<T0, epics::pvData::boolean>::value, flatbuffers::Offset<BrightnESS::ForwardEpicsToKafka::Epics::NTScalarArrayByte>, typename std::conditional<
	std::is_same<T0,  int8_t >::value, _O<_F::NTScalarArrayByte>,   typename std::conditional<
	std::is_same<T0,  int16_t>::value, _O<_F::NTScalarArrayShort>,   typename std::conditional<
	std::is_same<T0,  int32_t>::value, _O<_F::NTScalarArrayInt>,   typename std::conditional<
	std::is_same<T0,  int64_t>::value, _O<_F::NTScalarArrayLong>,   typename std::conditional<
	std::is_same<T0, uint8_t >::value, _O<_F::NTScalarArrayUByte>,   typename std::conditional<
	std::is_same<T0, uint16_t>::value, _O<_F::NTScalarArrayUShort>,   typename std::conditional<
	std::is_same<T0, uint32_t>::value, _O<_F::NTScalarArrayUInt>,   typename std::conditional<
	std::is_same<T0, uint64_t>::value, _O<_F::NTScalarArrayULong>,   typename std::conditional<
	std::is_same<T0, float   >::value, _O<_F::NTScalarArrayFloat>,  typename std::conditional<
	std::is_same<T0, double  >::value, _O<_F::NTScalarArrayDouble>, std::nullptr_t
#undef _F
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


// Flat buffers is explicit about the number types.  Epics not.  Need to translate.
// static_assert below keeps us sane.
using T3 = typename std::conditional<
	std::is_same<T0, epics::pvData::boolean>::value,  int8_t,   typename std::conditional<
	std::is_same<T0,  int8_t >::value,  int8_t ,   typename std::conditional<
	std::is_same<T0,  int16_t>::value,  int16_t,   typename std::conditional<
	std::is_same<T0,  int32_t>::value,  int32_t,   typename std::conditional<
	std::is_same<T0,  int64_t>::value,  int64_t,   typename std::conditional<
	std::is_same<T0, uint8_t >::value, uint8_t ,   typename std::conditional<
	std::is_same<T0, uint16_t>::value, uint16_t,   typename std::conditional<
	std::is_same<T0, uint32_t>::value, uint32_t,   typename std::conditional<
	std::is_same<T0, uint64_t>::value, uint64_t,   typename std::conditional<
	std::is_same<T0,    float>::value,    float,   typename std::conditional<
	std::is_same<T0,   double>::value,   double,   std::nullptr_t
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


FBT convert(uint64_t seq, uint64_t ts, TopicMappingSettings & tms, epics::pvData::PVStructure::shared_pointer & pvstr) override {
	static_assert(FLATBUFFERS_LITTLEENDIAN, "Optimization relies on little endianness");
	static_assert(sizeof(T0) == sizeof(T3), "Numeric types not compatible");
	// Build the flat buffer from scratch
	FBT fb( new BrightnESS::FlatBufs::FB( FlatBufs::Schema::Simple ) );
	auto builder = fb->builder.get();
	T2 array_fin;
	{
		// NOTE
		// We rely on that we inspect the Epics field correctly on first read, and that the
		// Epics server will not silently change type.
		auto f1 = pvstr->getSubField<epics::pvData::PVValueArray<T0>>("value");
		auto svec = f1->view();
		auto nlen = svec.size();
		//auto hex = binary_to_hex((char*)svec.data(), svec.size() * sizeof(T0));
		//LOG(1, "packing {} array: {:.{}}", typeid(T0).name(), hex.data(), hex.size());

		if (tms.is_chopper_TDCE) {
			nlen = svec.at(0) + 2;
			LOG(0, "Note: TDCE nlen: {}", nlen);
		}

		// Silence warning about char vs. signed char
		auto off_vec = builder->CreateVector((T3*)svec.data(), nlen);
		T1 array_builder(*builder);
		array_builder.add_value(off_vec);
		array_fin = array_builder.Finish();
	}

	// Adding name not moved yet into the add_name_timeStamp, because CreateString would be nested.
	// Therefore, create that string first.
	auto off_name = builder->CreateString(tms.channel);

	EpicsPVBuilder pv_builder(*builder);

	#if PAYLOAD_TESTING
	// Dummy payload for testing:
	class dummypayload : public std::vector<float> {
	public:
		dummypayload() {
			resize(256 * 1024);
		}
	};
	static dummypayload d2;
	auto off_d2 = builder->CreateVector(d2.data(), d2.size());
	pv_builder.add_d2(off_d2);
	#endif

	pv_builder.add_name(off_name);

	add_name_timeStamp(*builder, pv_builder, tms.channel, pvstr);
	pv_builder.add_pv_type(BuilderType_to_Enum_PV<T1>::v());
	pv_builder.add_pv(array_fin.Union());

	pv_builder.add_seq(seq);
	pv_builder.add_ts(ts);
	pv_builder.add_ts_epics_server(pvstr->getSubField<epics::pvData::PVScalarValue<uint64_t>>("ts")->get());

	builder->Finish(pv_builder.Finish());

	return fb;
}

};

}



template <typename ...TX>
struct PVStructureToFlatBuffer_create;

template <typename T0>
struct PVStructureToFlatBuffer_create<T0> {
static PVStructureToFlatBuffer::ptr impl(epics::pvData::PVField::shared_pointer & pv_value) {
	if (dynamic_cast<epics::pvData::PVScalarValue<T0>*>(pv_value.get())) {
		return PVStructureToFlatBuffer::ptr(new PVStructureToFlatBufferN::NTScalar<T0>);
	}
	return nullptr;
}
};

template <typename T0, typename T1, typename ...TX>
struct PVStructureToFlatBuffer_create<T0, T1, TX...> {
static PVStructureToFlatBuffer::ptr impl(epics::pvData::PVField::shared_pointer & pv_value) {
	if (auto x = PVStructureToFlatBuffer_create<T0>::impl(pv_value)) return x;
	return PVStructureToFlatBuffer_create<T1, TX...>::impl(pv_value);
}
};



template <typename ...TX>
struct PVStructureToFlatBuffer_create_array;

template <typename T0>
struct PVStructureToFlatBuffer_create_array<T0> {
static PVStructureToFlatBuffer::ptr impl(epics::pvData::PVField::shared_pointer & pv_value) {
	if (dynamic_cast<epics::pvData::PVValueArray<T0>*>(pv_value.get())) {
		return PVStructureToFlatBuffer::ptr(new PVStructureToFlatBufferN::NTScalarArray<T0>);
	}
	return nullptr;
}
};

template <typename T0, typename T1, typename ...TX>
struct PVStructureToFlatBuffer_create_array<T0, T1, TX...> {
static PVStructureToFlatBuffer::ptr impl(epics::pvData::PVField::shared_pointer & pv_value) {
	if (auto x = PVStructureToFlatBuffer_create_array<T0>::impl(pv_value)) return x;
	return PVStructureToFlatBuffer_create_array<T1, TX...>::impl(pv_value);
}
};



PVStructureToFlatBuffer::ptr PVStructureToFlatBuffer::create(epics::pvData::PVStructure::shared_pointer & pvstr) {
	auto id = pvstr->getField()->getID();
	LOG(9, "pvstr->getField()->getID(): {}", id.c_str());
	auto pv_value = pvstr->getSubField("value");
	if (!pv_value) {
		LOG(5, "ERROR PVField has no subfield 'value'");
		return nullptr;
	}
	// Pull in the epics::pvData::boolean type:
	using namespace epics::pvData;
	if (id == "epics:nt/NTScalar:1.0") {
		if (auto x = PVStructureToFlatBuffer_create<
			// List of types from EPICS pv/pvData.h , search for PVUByte
			 int8_t,
			 int16_t,
			 int32_t,
			 int64_t,
			uint8_t,
			uint16_t,
			uint32_t,
			uint64_t,
			float,
			double,
			epics::pvData::boolean
			>::impl(pv_value)) {
				return x;
		}
		LOG(5, "ERROR unknown NTScalar type");
	}
	else if (id == "epics:nt/NTScalarArray:1.0") {
		//LOG(9, "epics:nt/NTScalarArray:1.0");
		if (auto x = PVStructureToFlatBuffer_create_array<
			 int8_t,
			 int16_t,
			 int32_t,
			 int64_t,
			uint8_t,
			uint16_t,
			uint32_t,
			uint64_t,
			float,
			double,
			epics::pvData::boolean
			>::impl(pv_value)) {
				return x;
		}
		LOG(5, "ERROR unknown NTScalarArray type");
	}
	return nullptr;
}
