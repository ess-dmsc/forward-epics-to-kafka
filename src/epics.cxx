/** \file
Implements the requester classes that Epics uses as callbacks.
Be careful with exceptions as the Epics C++ API does not clearly state
from which threads the callbacks are invoked.
Instead, we call the supervising class which in turn stops operation
and marks the mapping as failed.  Cleanup is then triggered in the main
watchdog thread.
*/

#include "epics.h"
#include "logger.h"
#include "TopicMapping.h"
#include <exception>
#include <random>
#include <type_traits>
#include "local_config.h"
#include "helper.h"
#include <vector>

// For NTNDArray tests:
#include <pv/nt.h>
#include <pv/ntndarray.h>
#include <pv/ntndarrayAttribute.h>
#include <pv/ntutils.h>

#include "fbhelper.h"
#include "fbschemas.h"

namespace BrightnESS {
namespace ForwardEpicsToKafka {

namespace Epics {

using std::string;
using epics::pvData::Structure;
using epics::pvData::PVStructure;
using epics::pvData::Field;
using epics::pvData::MessageType;
using epics::pvAccess::Channel;

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


char const * channel_state_name(epics::pvAccess::Channel::ConnectionState x) {
#define DWTN1(N) DWTN2(N, STRINGIFY(N))
#define DWTN2(N, S) if (x == epics::pvAccess::Channel::ConnectionState::N) { return S; }
	DWTN1(NEVER_CONNECTED);
	DWTN1(CONNECTED);
	DWTN1(DISCONNECTED);
	DWTN1(DESTROYED);
#undef DWTN1
#undef DWTN2
	return "[unknown]";
}


#include <fmt/format.h>
#define FLOG(level, fmt, args...)  print("{:{}s}" fmt "\n", "", 2*(level), ## args);


namespace fbg {
	using std::vector;
	using fmt::print;
	typedef struct { int type; flatbuffers::Offset<void> off; } F_t;

	F_t F(flatbuffers::FlatBufferBuilder & builder, epics::pvData::PVFieldPtr const & field, int level) {
		auto etype = field->getField()->getType();
		if (etype == epics::pvData::Type::structure) {
			auto pvstr = dynamic_cast<epics::pvData::PVStructure*>(field.get());
			auto & subfields = pvstr->getPVFields();
			FLOG(level, "subfields.size(): {}", subfields.size());

			// For each subfield, collect the offsets:
			vector<F_t> fs;

			for (auto & f1ptr : subfields) {
				fs.push_back(F(builder, f1ptr, 1+level));
			}

			for (auto & x : fs) {
				FLOG(level, "off: {}", x.off.o);
			}
		}
		else if (etype == epics::pvData::Type::structureArray) {
		}
		else if (etype == epics::pvData::Type::scalar) {
			FLOG(level, "scalar");
			//builder->CreateInt(12);

			// TODO
			// Need this for all types...
			// This is just a dummy:
			pvIntBuilder b1(builder);
			b1.add_v(123);
			//auto x1 = static_cast<flatbuffers::Offset<void>>(b1.Finish());
			auto k1 = b1.Finish();
			auto k2 = k1.Union();
			//PVBuilder b2(builder);
			//b2.add_v_type(F_pvInt);
			//b2.add_v(k2);
			return { F_pvInt, k1.Union() };
		}
		else if (etype == epics::pvData::Type::scalarArray) {
		}
		else if (etype == epics::pvData::Type::union_) {
		}
		else if (etype == epics::pvData::Type::unionArray) {
		}
		else {
			throw std::runtime_error("Somethings wrong, none of the known types match");
		}
	}
}

FBBptr conv_to_fb_general(TopicMappingSettings const & settings, epics::pvData::PVStructure::shared_pointer & pvstr) {
	LOG(3, "conv_to_fb_general");
	// Passing initial size:
	auto builder = new flatbuffers::FlatBufferBuilder(1024);
	auto off = fbg::F(*builder, pvstr, 0);
	//some kind of 'union F' offset:   flatbuffers::Offset<void>
	PVBuilder b1(*builder);
	return FBBptr(builder);
}





class PVStructureToFlatBuffer {
public:
using FBT = FBBptr;
using ptr = std::unique_ptr<PVStructureToFlatBuffer>;
static ptr create(epics::pvData::PVStructure::shared_pointer & pvstr);
virtual FBT convert(TopicMappingSettings & settings, epics::pvData::PVStructure::shared_pointer & pvstr) = 0;
};

namespace PVStructureToFlatBufferN {


void add_name_timeStamp(flatbuffers::FlatBufferBuilder & b1, EpicsPVBuilder & b2, std::string & channel_name, epics::pvData::PVStructure::shared_pointer & pvstr) {
	auto ts = pvstr->getSubField<epics::pvData::PVStructure>("timeStamp");
	if (not ts) {
		LOG(0, "timeStamp not available");
		return;
	}
	timeStamp_t timeStamp(
		ts->getSubField<epics::pvData::PVScalarValue<int64_t>>("secondsPastEpoch")->get(),
		ts->getSubField<epics::pvData::PVScalarValue<int>>("nanoseconds")->get()
	);
	//LOG(5, "secondsPastEpoch: {:20}", timeStamp.secondsPastEpoch());
	//LOG(5, "nanoseconds:      {:20}", timeStamp.nanoseconds());
	b2.add_timeStamp(&timeStamp);
}



struct Enum_PV_Base {
using RET = BrightnESS::ForwardEpicsToKafka::Epics::PV;
};

template <typename T0> struct BuilderType_to_Enum_PV : public Enum_PV_Base { static RET v(); };
template <> struct BuilderType_to_Enum_PV<NTScalarByteBuilder>        : public Enum_PV_Base { static RET v() { return PV_NTScalarByte; } };
template <> struct BuilderType_to_Enum_PV<NTScalarUByteBuilder>       : public Enum_PV_Base { static RET v() { return PV_NTScalarUByte; } };
template <> struct BuilderType_to_Enum_PV<NTScalarShortBuilder>       : public Enum_PV_Base { static RET v() { return PV_NTScalarShort; } };
template <> struct BuilderType_to_Enum_PV<NTScalarUShortBuilder>      : public Enum_PV_Base { static RET v() { return PV_NTScalarUShort; } };
template <> struct BuilderType_to_Enum_PV<NTScalarIntBuilder>         : public Enum_PV_Base { static RET v() { return PV_NTScalarInt; } };
template <> struct BuilderType_to_Enum_PV<NTScalarUIntBuilder>        : public Enum_PV_Base { static RET v() { return PV_NTScalarUInt; } };
template <> struct BuilderType_to_Enum_PV<NTScalarLongBuilder>        : public Enum_PV_Base { static RET v() { return PV_NTScalarLong; } };
template <> struct BuilderType_to_Enum_PV<NTScalarULongBuilder>       : public Enum_PV_Base { static RET v() { return PV_NTScalarULong; } };
template <> struct BuilderType_to_Enum_PV<NTScalarFloatBuilder>       : public Enum_PV_Base { static RET v() { return PV_NTScalarFloat; } };
template <> struct BuilderType_to_Enum_PV<NTScalarDoubleBuilder>      : public Enum_PV_Base { static RET v() { return PV_NTScalarDouble; } };

template <> struct BuilderType_to_Enum_PV<NTScalarArrayByteBuilder>   : public Enum_PV_Base { static RET v() { return PV_NTScalarArrayByte; } };
template <> struct BuilderType_to_Enum_PV<NTScalarArrayUByteBuilder>  : public Enum_PV_Base { static RET v() { return PV_NTScalarArrayUByte; } };
template <> struct BuilderType_to_Enum_PV<NTScalarArrayShortBuilder>  : public Enum_PV_Base { static RET v() { return PV_NTScalarArrayShort; } };
template <> struct BuilderType_to_Enum_PV<NTScalarArrayUShortBuilder> : public Enum_PV_Base { static RET v() { return PV_NTScalarArrayUShort; } };
template <> struct BuilderType_to_Enum_PV<NTScalarArrayIntBuilder>    : public Enum_PV_Base { static RET v() { return PV_NTScalarArrayInt; } };
template <> struct BuilderType_to_Enum_PV<NTScalarArrayUIntBuilder>   : public Enum_PV_Base { static RET v() { return PV_NTScalarArrayUInt; } };
template <> struct BuilderType_to_Enum_PV<NTScalarArrayLongBuilder>   : public Enum_PV_Base { static RET v() { return PV_NTScalarArrayLong; } };
template <> struct BuilderType_to_Enum_PV<NTScalarArrayULongBuilder>  : public Enum_PV_Base { static RET v() { return PV_NTScalarArrayULong; } };
template <> struct BuilderType_to_Enum_PV<NTScalarArrayFloatBuilder>  : public Enum_PV_Base { static RET v() { return PV_NTScalarArrayFloat; } };
template <> struct BuilderType_to_Enum_PV<NTScalarArrayDoubleBuilder> : public Enum_PV_Base { static RET v() { return PV_NTScalarArrayDouble; } };





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

FBT convert(TopicMappingSettings & tms, epics::pvData::PVStructure::shared_pointer & pvstr) override {
	auto builder = FBT(new flatbuffers::FlatBufferBuilder);
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
	builder->Finish(pv_builder.Finish());

	return builder;
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


FBT convert(TopicMappingSettings & tms, epics::pvData::PVStructure::shared_pointer & pvstr) override {
	static_assert(FLATBUFFERS_LITTLEENDIAN, "Optimization relies on little endianness");
	static_assert(sizeof(T0) == sizeof(T3), "Numeric types not compatible");
	// Build the flat buffer from scratch
	auto builder = FBT(new flatbuffers::FlatBufferBuilder);
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
	builder->Finish(pv_builder.Finish());

	return builder;
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




class ActionOnChannel {
public:
ActionOnChannel(Monitor::wptr monitor) : monitor(monitor) { }
virtual void operator () (epics::pvAccess::Channel::shared_pointer const & channel) {
	LOG(5, "[EMPTY ACTION]");
};
Monitor::wptr monitor;
};



//                ChannelRequester

// Listener for channel state changes
class ChannelRequester : public epics::pvAccess::ChannelRequester {
public:
ChannelRequester(std::unique_ptr<ActionOnChannel> action);

// From class pvData::Requester
std::string getRequesterName() override;
void message(std::string const & message, MessageType messageType) override;

void channelCreated(const epics::pvData::Status& status, epics::pvAccess::Channel::shared_pointer const & channel) override;
void channelStateChange(epics::pvAccess::Channel::shared_pointer const & channel, epics::pvAccess::Channel::ConnectionState connectionState) override;

private:
//GetFieldRequesterDW::shared_pointer gfr;
//epics::pvAccess::ChannelGetRequester::shared_pointer cgr;
//ChannelGet::shared_pointer cg;

std::unique_ptr<ActionOnChannel> action;

// Monitor operation:
epics::pvData::MonitorRequester::shared_pointer monr;
epics::pvData::MonitorPtr mon;
};





ChannelRequester::ChannelRequester(std::unique_ptr<ActionOnChannel> action) :
	action(std::move(action))
{
}

string ChannelRequester::getRequesterName() {
	return "ChannelRequester";
}

void ChannelRequester::message(std::string const & message, MessageType messageType) {
	LOG(3, "Message for: {}  msg: {}  msgtype: {}", getRequesterName().c_str(), message.c_str(), getMessageTypeName(messageType).c_str());
}



/*
Seems that channel creation is actually a synchronous operation
and that this requester callback is called from the same stack
from which the channel creation was initiated.
*/

void ChannelRequester::channelCreated(epics::pvData::Status const & status, Channel::shared_pointer const & channel) {
	auto monitor = action->monitor.lock();
	if (not monitor) {
		LOG(9, "ERROR Assertion failed:  Expect to get a shared_ptr to the monitor");
	}
	#if TEST_RANDOM_FAILURES
		std::mt19937 rnd(std::chrono::system_clock::now().time_since_epoch().count());
		if (rnd() < 0.1 * 0xffffffff) {
			if (monitor) monitor->go_into_failure_mode();
		}
	#endif
	LOG(0, "ChannelRequester::channelCreated:  (int)status.isOK(): {}", (int)status.isOK());
	if (!status.isOK() or !status.isSuccess()) {
		// quick fix until decided on logging system..
		std::ostringstream s1;
		s1 << status;
		LOG(3, "WARNING ChannelRequester::channelCreated:  {}", s1.str().c_str());
	}
	if (!status.isSuccess()) {
		// quick fix until decided on logging system..
		std::ostringstream s1;
		s1 << status;
		LOG(6, "ChannelRequester::channelCreated:  failure: {}", s1.str().c_str());
		if (channel) {
			// Yes, take a copy
			std::string cname = channel->getChannelName();
			LOG(6, "  failure is in channel: {}", cname.c_str());
		}
		if (monitor) monitor->go_into_failure_mode();
	}
}

void ChannelRequester::channelStateChange(Channel::shared_pointer const & channel, Channel::ConnectionState cstate) {
	auto monitor = action->monitor.lock();
	LOG(0, "channel state change: {}", Channel::ConnectionStateNames[cstate]);
	if (cstate == Channel::DISCONNECTED) {
		LOG(1, "Epics channel disconnect");
		if (monitor) monitor->go_into_failure_mode();
		return;
	}
	else if (cstate == Channel::DESTROYED) {
		LOG(1, "Epics channel destroyed");
		if (monitor) monitor->go_into_failure_mode();
		return;
	}
	else if (cstate != Channel::CONNECTED) {
		LOG(6, "Unhandled channel state change: {}", channel_state_name(cstate));
		if (monitor) monitor->go_into_failure_mode();
	}
	if (!channel) {
		LOG(6, "ERROR no channel, even though we should have.  state: {}", channel_state_name(cstate));
		if (monitor) monitor->go_into_failure_mode();
	}

	action->operator()(channel);
}




class ActionOnField {
public:
ActionOnField(Monitor::wptr monitor) : monitor(monitor) { }
virtual void operator () (Field const & field) { };
Monitor::wptr monitor;
};



class GetFieldRequesterForAction : public epics::pvAccess::GetFieldRequester {
public:
GetFieldRequesterForAction(epics::pvAccess::Channel::shared_pointer channel, std::unique_ptr<ActionOnField> action);
// from class epics::pvData::Requester
string getRequesterName() override;
void message(string const & msg, epics::pvData::MessageType msgT) override;
void getDone(epics::pvData::Status const & status, epics::pvData::FieldConstPtr const & field) override;
private:
std::unique_ptr<ActionOnField> action;
};

GetFieldRequesterForAction::GetFieldRequesterForAction(epics::pvAccess::Channel::shared_pointer channel, std::unique_ptr<ActionOnField> action) :
	action(std::move(action))
{
	LOG(0, STRINGIFY(GetFieldRequesterForAction) " ctor");
}

string GetFieldRequesterForAction::getRequesterName() { return STRINGIFY(GetFieldRequesterForAction); }

void GetFieldRequesterForAction::message(string const & msg, epics::pvData::MessageType msgT) {
	LOG(3, "GetFieldRequesterForAction::message: {}", msg.c_str());
}

void GetFieldRequesterForAction::getDone(epics::pvData::Status const & status, epics::pvData::FieldConstPtr const & field) {
	if (!status.isSuccess()) {
		LOG(6, "ERROR nosuccess");
		auto monitor = action->monitor.lock();
		if (monitor) {
			monitor->go_into_failure_mode();
		}
	}
	if (status.isOK()) {
		LOG(0, "success and OK");
	}
	else {
		LOG(3, "success with warning:  [[TODO STATUS]]");
	}
	action->operator()(*field);
}





// ============================================================================
// MONITOR
// The Monitor class is in pvData, they say that's because it only depends on pvData, so they have put
// it there, what a logic...
// Again, we have a MonitorRequester and :


class MonitorRequester : public ::epics::pvData::MonitorRequester {
public:
MonitorRequester(std::string channel_name, Monitor::wptr monitor_HL);
~MonitorRequester();
string getRequesterName() override;
void message(string const & msg, epics::pvData::MessageType msgT) override;

void monitorConnect(epics::pvData::Status const & status, epics::pvData::Monitor::shared_pointer const & monitor, epics::pvData::StructureConstPtr const & structure) override;

void monitorEvent(epics::pvData::MonitorPtr const & monitor) override;
void unlisten(epics::pvData::MonitorPtr const & monitor) override;

private:
std::string m_channel_name;
//epics::pvData::MonitorPtr monitor;
Monitor::wptr monitor_HL;
PVStructureToFlatBuffer::ptr conv_to_flatbuffer;
};

MonitorRequester::MonitorRequester(std::string channel_name, Monitor::wptr monitor_HL) :
	m_channel_name(channel_name),
	monitor_HL(monitor_HL)
{
}


MonitorRequester::~MonitorRequester() {
	LOG(0, "dtor");
}

string MonitorRequester::getRequesterName() { return "MonitorRequester"; }

void MonitorRequester::message(string const & msg, epics::pvData::MessageType msgT) {
	LOG(3, "MonitorRequester::message: {}", msg.c_str());
}


void MonitorRequester::monitorConnect(epics::pvData::Status const & status, epics::pvData::Monitor::shared_pointer const & monitor_, epics::pvData::StructureConstPtr const & structure) {
	auto monitor_HL = this->monitor_HL.lock();
	if (!status.isSuccess()) {
		// NOTE
		// Docs does not say anything about whether we are responsible for any handling of the monitor if non-null?
		LOG(6, "ERROR nosuccess");
		if (monitor_HL) {
			monitor_HL->go_into_failure_mode();
		}
	}
	else {
		if (status.isOK()) {
			LOG(0, "success and OK");
		}
		else {
			LOG(3, "success with warning:  [[TODO STATUS]]");
		}
	}
	//monitor = monitor_;
	monitor_->start();
}




void MonitorRequester::monitorEvent(epics::pvData::MonitorPtr const & monitor) {
	LOG(0, "got event");

	auto monitor_HL = this->monitor_HL.lock();
	if (!monitor_HL) {
		LOG(5, "monitor_HL already gone");
		return;
	}

	#if TEST_RANDOM_FAILURES
		std::mt19937 rnd(std::chrono::system_clock::now().time_since_epoch().count());
		if (rnd() < 0.12 * 0xffffffff) {
			monitor_HL->go_into_failure_mode();
		}
	#endif

	// Docs for MonitorRequester says that we have to call poll()
	// This seems weird naming to me, because I listen to the Monitor because I do not want to poll?!?
	while (auto ele = monitor->poll()) {

		// Seems like MonitorElement always returns a Structure type ?
		// The inheritance diagram shows that scalars derive from Field, not from Structure.
		// Does that mean that we never get a scalar here directly??

		auto & pvstr = ele->pvStructurePtr;

		bool const use_general =
		#ifdef FLATBUFFERS_USE_GENERAL
			true
		#else
			false
		#endif
			;
		if (use_general) {
			// try a new general but slower approach to cover all kinds of PV
			auto fb = conv_to_fb_general(monitor_HL->topic_mapping->topic_mapping_settings, pvstr);
		}
		else {
			// NOTE
			// One assumption is currently that we stream only certain types from EPICS,
			// including NTScalar and NTScalarArray.
			// This is implemneted currently by passing the PVStructure to this function which decides
			// based on the naming scheme what type it contains.
			// A more robust solution in the future should actually investigate the PVStructure.
			// Open question:  Could EPICS suddenly change the type during runtime?
			if (!conv_to_flatbuffer) conv_to_flatbuffer = PVStructureToFlatBuffer::create(pvstr);
			if (!conv_to_flatbuffer) {
				LOG(5, "ERROR can not create a converter to produce flat buffers for this field");
				monitor_HL->go_into_failure_mode();
			}
			else {
				auto flat_buffer = conv_to_flatbuffer->convert(monitor_HL->topic_mapping->topic_mapping_settings, pvstr);
				monitor_HL->emit(std::move(flat_buffer));
				monitor->release(ele);
			}
		}
	}
}

void MonitorRequester::unlisten(epics::pvData::MonitorPtr const & monitor) {
	LOG(3, "monitor source no longer available");
}









class StartMonitorChannel : public ActionOnChannel {
public:
StartMonitorChannel(Monitor::wptr monitor) : ActionOnChannel(monitor) { }

void operator () (epics::pvAccess::Channel::shared_pointer const & channel) override {
	auto monitor = this->monitor.lock();
	if (monitor) {
		monitor->initiate_value_monitoring();
	}
}

private:
epics::pvAccess::GetFieldRequester::shared_pointer gfr;
};








//                 Monitor

Monitor::Monitor(TopicMapping * topic_mapping, std::string channel_name) :
	topic_mapping(topic_mapping),
	channel_name(channel_name)
{
	if (!topic_mapping) throw std::runtime_error("ERROR did not receive a topic_mapping");
}

void Monitor::init(std::shared_ptr<Monitor> self) {
	this->self = decltype(this->self)(self);
	initiate_connection();
}

Monitor::~Monitor() {
	LOG(0, "Monitor dtor");
	stop();
}

bool Monitor::ready() {
	return ready_monitor;
}

void Monitor::stop() {
	RMLG lg(m_mutex_emitter);
	try {
		if (mon) {
			//LOG(5, "stopping monitor for TM {}", topic_mapping->id);

			// TODO
			// After some debugging, it seems to me that even though we call stop() on Epics
			// monitor, the Epics library will continue to transfer data even though the monitor
			// will not produce events anymore.
			// This continues, until the monitor object is destructed.
			// Based on this observation, this code could use some refactoring.

			// Even though I have a smart pointer, calling stop() sometimes causes segfault within
			// Epics under load.  Not good.
			//mon->stop();

			// Docs say, that one must call destroy(), and it calims that 'delete' is prevented,
			// but that can't be true.
			mon->destroy();
			mon.reset();
		}
	}
	catch (std::runtime_error & e) {
		LOG(5, "Runtime error from Epics: {}", e.what());
		go_into_failure_mode();
	}
}


/**
A crutch to cope with Epics delayed release of resources.  Refactor...
*/
void Monitor::topic_mapping_gone() {
	RMLG lg(m_mutex_emitter);
	topic_mapping = nullptr;
}


void Monitor::go_into_failure_mode() {
	// Can be called from different threads, make sure we trigger only once.
	// Can also be called recursive, from different error conditions.

	// TODO
	// How to be sure that no callbacks are invoked any longer?
	// Does Epics itself provide a facility for that already?
	// Maybe not clear.  Epics invokes the channel state change sometimes really late
	// from the delayed deleter process.  (no documentation about this avail?)

	RMLG lg(m_mutex_emitter);

	if (failure_triggered.exchange(1) == 0) {
		stop();
		if (topic_mapping) {
			if (topic_mapping->forwarding) {
				topic_mapping->go_into_failure_mode();
			}
		}
	}
}


void Monitor::initiate_connection() {
	static bool do_init_factory_pva { true };
	static bool do_init_factory_ca  { true };
	//static epics::pvAccess::ChannelProviderRegistry::shared_pointer provider;

	// Not yet clear how many codepaths will depend on this
	auto & t = topic_mapping->topic_mapping_settings.type;
	using T = TopicMappingType;



	if      (t == T::EPICS_PVA_NT) {
		if (do_init_factory_pva) {
			epics::pvAccess::ClientFactory::start();
			do_init_factory_pva = false;
		}
		provider = epics::pvAccess::getChannelProviderRegistry()
			->getProvider("pva");
	}
	else if (t == T::EPICS_CA_VALUE) {
		if (do_init_factory_ca) {
			epics::pvAccess::ca::CAClientFactory::start();
			do_init_factory_ca = true;
		}
		provider = epics::pvAccess::getChannelProviderRegistry()
			->getProvider("ca");
	}
	if (provider == nullptr) {
		LOG(3, "ERROR could not create a provider");
		throw epics_channel_failure();
	}
	//cr.reset(new ChannelRequester(std::move(new StartMonitorChannel())));
	cr.reset(new ChannelRequester(std::unique_ptr<StartMonitorChannel>(new StartMonitorChannel(self) )));
	ch = provider->createChannel(channel_name, cr);
}


void Monitor::initiate_value_monitoring() {
	RMLG lg(m_mutex_emitter);
	if (!topic_mapping) return;
	if (!topic_mapping->forwarding) return;
	monr.reset(new MonitorRequester(channel_name, self));

	// Leaving it empty seems to be the full channel, including name.  That's good.
	// Can also specify subfields, e.g. "value, timeStamp"  or also "field(value)"
	string request = "";
	PVStructure::shared_pointer pvreq;
	pvreq = epics::pvData::CreateRequest::create()->createRequest(request);
	mon = ch->createMonitor(monr, pvreq);
	if (!mon) {
		throw std::runtime_error("ERROR could not create EPICS monitor instance");
	}
	if (mon) {
		ready_monitor = true;
	}
}

void Monitor::emit(FBBptr fbuf) {
	// TODO
	// It seems to be hard to tell when Epics won't use the callback anymore.
	// Instead of checking each access, use flags and quarantine before release
	// of memory.
	RMLG lg(m_mutex_emitter);
	if (!topic_mapping) return;
	if (!topic_mapping->forwarding) return;
	topic_mapping->emit(std::move(fbuf));
}






}
}
}



int epics_test_fb_general() {
	auto pvstr = epics::nt::NTNDArray::createBuilder()
	->addAlarm()
	->createPVStructure();
	auto const type = epics::pvData::ScalarType::pvFloat;
	// Note how we have to specify the basic scalar type here:
	auto a = epics::pvData::getPVDataCreate()->createPVScalarArray<epics::pvData::PVValueArray<float>>();
	// Fill with dummy data:
	a->setLength(16);
	//int xx = a;
	auto a1 = a->reuse();
	for (auto & x : a1) { x = 0.1; }
	a->replace(epics::pvData::freeze(a1));
	if (auto u = dynamic_cast<epics::pvData::PVUnion*>(pvstr->getSubField("value").get())) {
		auto n = std::string(epics::pvData::ScalarTypeFunc::name(type)) + "Value";
		u->set(n, a);
	}
	pvstr->dumpValue(std::cout);
	BrightnESS::ForwardEpicsToKafka::Epics::conv_to_fb_general(BrightnESS::ForwardEpicsToKafka::TopicMappingSettings("ch1", "tp1"), pvstr);
	return 0;
}
