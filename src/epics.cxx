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
#include "config.h"
#include "helper.h"


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


char const * field_type_name(epics::pvData::Type x) {
#define DWTN1(N) DWTN2(N, STRINGIFY(N))
#define DWTN2(N, S) if (x == epics::pvData::N) { return S; }
	DWTN1(scalar);
	DWTN1(scalarArray);
	DWTN1(structure);
	DWTN1(structureArray);
	DWTN2(union_, "union");
	DWTN1(unionArray);
#undef DWTN1
#undef DWTN2
	return "[unknown]";
}

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


// Use a leight-weight LOG for the inspection
#define FLOG(level, fmt, args...) printf("%*s" fmt "\n", 2*level, "", ## args);


char const * pv_scalar_type_name(epics::pvData::ScalarType type) {
	#define M(T) if (type == epics::pvData::T) return STRINGIFY(T);
	M(pvBoolean);
	M(pvByte);
	M(pvShort);
	M(pvInt);
	M(pvLong);
	M(pvUByte);
	M(pvUShort);
	M(pvUInt);
	M(pvULong);
	M(pvFloat);
	M(pvDouble);
	M(pvString);
	#undef M
	return "unknown";
}


void inspect_Structure(Structure const & str, int level = 0);


void inspect_Field(epics::pvData::Field const & field, int level = 0) {
	using namespace epics::pvData;
	FLOG(level, "inspect_Field  getID(): %s   getType(): %s",
		field.getID().c_str(), field_type_name(field.getType()));
	if (auto p1 = dynamic_cast<Structure const *>(&field)) {
		inspect_Structure(*p1, level);
	}
	else if (auto p1 = dynamic_cast<epics::pvData::Scalar const *>(&field)) {
		FLOG(level, "{scalar : %s}", pv_scalar_type_name(p1->getScalarType()));
	}
	else {
		FLOG(level, "[not handled]");
	}
}


void inspect_Structure(epics::pvData::Structure const & str, int level) {
	auto & subfields = str.getFields();
	FLOG(level, "inspect_Structure  getNumberFields: %lu  subfields.size(): %lu",
		str.getNumberFields(),
		subfields.size()
	);
	int i1 = 0;
	for (auto & f1sptr : subfields) {
		auto & f1 = *f1sptr.get();
		FLOG(level, "now inspect subfield [%s]", str.getFieldName(i1).c_str());
		inspect_Field(f1, 1+level);
		++i1;
	}
}




void inspect_PVStructure(epics::pvData::PVStructure const & pvstr, int level = 0);


void inspect_PVField(epics::pvData::PVField const & field, int level = 0) {
	// TODO
	// After this initial 'getting-to-know-Epics' refactor into polymorphic access
	FLOG(level, "inspect_PVField  getFieldName: %s  getFullName: %s  getField()->getID(): %s  getNumberFields: %lu",
		field.getFieldName().c_str(),
		field.getFullName().c_str(),
		field.getField()->getID().c_str(),
		field.getNumberFields()
	);
	if (auto p1 = dynamic_cast<PVStructure const *>(&field)) {
		inspect_PVStructure(*p1, level);
		if (false) {
			// Older tests:
			// One possibility:  get the Field
			LOG(0, "%lx", p1->getField().get());
			LOG(0, "ele->pvStructurePtr->getField()->getID(): %s", p1->getField()->getID().c_str());
		}
	}
	else if (auto p1 = dynamic_cast<epics::pvData::PVScalarValue<double> const *>(&field)) {
		FLOG(level, "[  double == % e]", p1->get());
	}
	else if (auto p1 = dynamic_cast<epics::pvData::PVScalarValue< int32_t> const *>(&field)) {
		FLOG(level, "[ int32_t == % d]", p1->get());
	}
	else if (auto p1 = dynamic_cast<epics::pvData::PVScalarValue<uint32_t> const *>(&field)) {
		FLOG(level, "[uint32_t == % d]", p1->get());
	}
	else if (auto p1 = dynamic_cast<epics::pvData::PVScalarValue< int64_t> const *>(&field)) {
		FLOG(level, "[ int64_t == % ld]", p1->get());
	}
	else if (auto p1 = dynamic_cast<epics::pvData::PVScalarValue<uint64_t> const *>(&field)) {
		FLOG(level, "[uint64_t == % ld]", p1->get());
	}
	else if (auto p1 = dynamic_cast<epics::pvData::PVScalarValue<std::string> const *>(&field)) {
		FLOG(level, "[  string == %s]", p1->get().c_str());
	}
	else {
		FLOG(level, "[can not handle this]");
	}
}




void inspect_PVStructure(epics::pvData::PVStructure const & pvstr, int level) {
	FLOG(level, "inspect_PVStructure");
	// getStructure is only on PVStructure, not on PVField
	//auto & str = *pvstr.getStructure();

	auto & subfields = pvstr.getPVFields();
	FLOG(level, "subfields.size(): %lu", subfields.size());
	for (auto & f1ptr : subfields) {
		auto & f1 = *f1ptr;
		inspect_PVField(f1, 1+level);
	}
}



struct FBpack {
char * data;
size_t len;
};


class PVStructureToFlatBuffer {
public:
using FBT = FBBptr;
using ptr = std::unique_ptr<PVStructureToFlatBuffer>;
static ptr create(epics::pvData::PVStructure::shared_pointer pvstr);
virtual FBT convert(epics::pvData::PVStructure::shared_pointer & pvstr) = 0;
};

namespace PVStructureToFlatBufferN {


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
	std::is_same<T0,          char  >::value, NTScalarByteBuilder,   typename std::conditional<
	std::is_same<T0, unsigned char  >::value, NTScalarUByteBuilder,  typename std::conditional<
	std::is_same<T0,          short >::value, NTScalarShortBuilder,  typename std::conditional<
	std::is_same<T0, unsigned short >::value, NTScalarUShortBuilder, typename std::conditional<
	std::is_same<T0,          int   >::value, NTScalarIntBuilder,    typename std::conditional<
	std::is_same<T0, unsigned int   >::value, NTScalarUIntBuilder,   typename std::conditional<
	std::is_same<T0,          long  >::value, NTScalarLongBuilder,   typename std::conditional<
	std::is_same<T0, unsigned long  >::value, NTScalarULongBuilder,  typename std::conditional<
	std::is_same<T0,          float >::value, NTScalarFloatBuilder,  typename std::conditional<
	std::is_same<T0,          double>::value, NTScalarDoubleBuilder, nullptr_t
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

FBT convert(epics::pvData::PVStructure::shared_pointer & pvstr) override {
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
		//LOG(1, "packing %s: %.*s", typeid(T0).name(), hex.size(), hex.data());
		scalar_builder.add_value(val);
	}
	auto scalar_fin = scalar_builder.Finish();

	EpicsPVBuilder pv_builder(*builder);
	add_timeStamp(pv_builder, pvstr);
	pv_builder.add_pv_type(BuilderType_to_Enum_PV<T1>::v());
	pv_builder.add_pv(scalar_fin.Union());
	builder->Finish(pv_builder.Finish());

	return builder;
}

void add_timeStamp(EpicsPVBuilder & b2, epics::pvData::PVStructure::shared_pointer & pvstr) {
	auto ts = pvstr->getSubField<epics::pvData::PVStructure>("timeStamp");
	timeStamp_t timeStamp(
		ts->getSubField<epics::pvData::PVScalarValue<long>>("secondsPastEpoch")->get(),
		ts->getSubField<epics::pvData::PVScalarValue<int>>("nanoseconds")->get()
	);
	//LOG(5, "secondsPastEpoch: %20ld", timeStamp.secondsPastEpoch());
	//LOG(5, "nanoseconds:      %20ld", timeStamp.nanoseconds());
	b2.add_timeStamp(&timeStamp);
}

};




template <typename T0>
class NTScalarArray : public PVStructureToFlatBuffer {
public:
using T1 = typename std::conditional<
	std::is_same<T0,          char  >::value, NTScalarArrayByteBuilder,   typename std::conditional<
	std::is_same<T0, unsigned char  >::value, NTScalarArrayUByteBuilder,  typename std::conditional<
	std::is_same<T0,          short >::value, NTScalarArrayShortBuilder,  typename std::conditional<
	std::is_same<T0, unsigned short >::value, NTScalarArrayUShortBuilder, typename std::conditional<
	std::is_same<T0,          int   >::value, NTScalarArrayIntBuilder,    typename std::conditional<
	std::is_same<T0, unsigned int   >::value, NTScalarArrayUIntBuilder,   typename std::conditional<
	std::is_same<T0,          long  >::value, NTScalarArrayLongBuilder,   typename std::conditional<
	std::is_same<T0, unsigned long  >::value, NTScalarArrayULongBuilder,  typename std::conditional<
	std::is_same<T0,          float >::value, NTScalarArrayFloatBuilder,  typename std::conditional<
	std::is_same<T0,          double>::value, NTScalarArrayDoubleBuilder, nullptr_t
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

using T2 = typename std::conditional<
	std::is_same<T0,          char  >::value, flatbuffers::Offset<BrightnESS::ForwardEpicsToKafka::Epics::NTScalarArrayByte>,   typename std::conditional<
	std::is_same<T0, unsigned char  >::value, flatbuffers::Offset<BrightnESS::ForwardEpicsToKafka::Epics::NTScalarArrayUByte>,  typename std::conditional<
	std::is_same<T0,          short >::value, flatbuffers::Offset<BrightnESS::ForwardEpicsToKafka::Epics::NTScalarArrayShort>,  typename std::conditional<
	std::is_same<T0, unsigned short >::value, flatbuffers::Offset<BrightnESS::ForwardEpicsToKafka::Epics::NTScalarArrayUShort>, typename std::conditional<
	std::is_same<T0,          int   >::value, flatbuffers::Offset<BrightnESS::ForwardEpicsToKafka::Epics::NTScalarArrayInt>,    typename std::conditional<
	std::is_same<T0, unsigned int   >::value, flatbuffers::Offset<BrightnESS::ForwardEpicsToKafka::Epics::NTScalarArrayUInt>,   typename std::conditional<
	std::is_same<T0,          long  >::value, flatbuffers::Offset<BrightnESS::ForwardEpicsToKafka::Epics::NTScalarArrayLong>,   typename std::conditional<
	std::is_same<T0, unsigned long  >::value, flatbuffers::Offset<BrightnESS::ForwardEpicsToKafka::Epics::NTScalarArrayULong>,  typename std::conditional<
	std::is_same<T0,          float >::value, flatbuffers::Offset<BrightnESS::ForwardEpicsToKafka::Epics::NTScalarArrayFloat>,  typename std::conditional<
	std::is_same<T0,          double>::value, flatbuffers::Offset<BrightnESS::ForwardEpicsToKafka::Epics::NTScalarArrayDouble>, nullptr_t
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
	std::is_same<T0,          char  >::value,  int8_t,   typename std::conditional<
	std::is_same<T0, unsigned char  >::value, uint8_t,   typename std::conditional<
	std::is_same<T0,          short >::value,  int16_t,  typename std::conditional<
	std::is_same<T0, unsigned short >::value, uint16_t,  typename std::conditional<
	std::is_same<T0,          int   >::value,  int32_t,  typename std::conditional<
	std::is_same<T0, unsigned int   >::value, uint32_t,  typename std::conditional<
	std::is_same<T0,          long  >::value,  int64_t,  typename std::conditional<
	std::is_same<T0, unsigned long  >::value, uint64_t,  typename std::conditional<
	std::is_same<T0,          float >::value,    float,  typename std::conditional<
	std::is_same<T0,          double>::value,   double,  nullptr_t
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


FBT convert(epics::pvData::PVStructure::shared_pointer & pvstr) override {
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
		//auto hex = binary_to_hex((char*)svec.data(), svec.size() * sizeof(T0));
		//LOG(1, "packing %s array: %.*s", typeid(T0).name(), hex.size(), hex.data());
		// Silence warning about char vs. signed char
		auto off_vec = builder->CreateVector((T3*)svec.data(), svec.size());
		T1 array_builder(*builder);
		array_builder.add_value(off_vec);
		array_fin = array_builder.Finish();
	}

	EpicsPVBuilder pv_builder(*builder);
	add_timeStamp(pv_builder, pvstr);
	pv_builder.add_pv_type(BuilderType_to_Enum_PV<T1>::v());
	pv_builder.add_pv(array_fin.Union());
	builder->Finish(pv_builder.Finish());

	return builder;
}

void add_timeStamp(EpicsPVBuilder & b2, epics::pvData::PVStructure::shared_pointer & pvstr) {
	auto ts = pvstr->getSubField<epics::pvData::PVStructure>("timeStamp");
	timeStamp_t timeStamp(
		ts->getSubField<epics::pvData::PVScalarValue<long>>("secondsPastEpoch")->get(),
		ts->getSubField<epics::pvData::PVScalarValue<int>>("nanoseconds")->get()
	);
	//LOG(5, "secondsPastEpoch: %20ld", timeStamp.secondsPastEpoch());
	//LOG(5, "nanoseconds:      %20ld", timeStamp.nanoseconds());
	b2.add_timeStamp(&timeStamp);
}

};





// Want partial specialization, therefore struct

using IN  = epics::pvData::PVField *;
using RET = PVStructureToFlatBuffer::ptr;

template <typename ...TX> struct Impl;

template <typename T0, typename T1, typename ...TX>
struct Impl<T0, T1, TX...> {
static RET create_ntscalar(IN f1) {
	if (auto x = Impl<T0>::create_ntscalar(f1)) return x;
	else return Impl<T1, TX...>::create_ntscalar(f1);
}
};


template <typename T0>
struct Impl<T0> {
static RET create_ntscalar(IN f1) {
	return RET(new NTScalar<T0>);
}
};


/*
template <>
struct Impl<float> {
using T0 = float;
static RET create_ntscalar(IN f1) {
	// TODO remove, do not need specialization here...
	return nullptr;
}
};
*/


template <>
struct Impl<> {
static RET create_ntscalar(IN f1) {
	//static_assert(false, "should not use this template");
	throw std::runtime_error("ERROR should not get invoked");
}
};


}



/*template <typename T0>
static PVStructureToFlatBuffer::ptr create_ntscalar<double>(epics::pvData::PVField * f1) {
	if (auto x = dynamic_cast<epics::pvData::PVScalarValue<T0>*>(f1)) {
		return PVStructureToFlatBuffer::ptr(new PVStructureToFlatBufferN::NTScalar<T0>);
	}
	return nullptr;
}*/


PVStructureToFlatBuffer::ptr PVStructureToFlatBuffer::create(epics::pvData::PVStructure::shared_pointer pvstr) {
	auto id = pvstr->getField()->getID();
	if (id == "epics:nt/NTScalar:1.0") {
		if (auto pv_value = pvstr->getSubField("value")) {
			#define MA(T) \
				if (dynamic_cast<epics::pvData::PVScalarValue<T>*>(pv_value.get())) { \
					return PVStructureToFlatBuffer::ptr(new PVStructureToFlatBufferN::NTScalar<T>); \
				}
			MA(         char  )
			MA(unsigned char  )
			MA(         short )
			MA(unsigned short )
			MA(         int   )
			MA(unsigned int   )
			MA(         long  )
			MA(unsigned long  )
			MA(         float )
			MA(         double)
			#undef MA
		}
		throw std::runtime_error("ERROR can not convert this NTScalar");
	}
	if (id == "epics:nt/NTScalarArray:1.0") {
		if (auto pv_value = pvstr->getSubField("value")) {
			#define MA(T) \
				if (dynamic_cast<epics::pvData::PVValueArray<T>*>(pv_value.get())) { \
					return PVStructureToFlatBuffer::ptr(new PVStructureToFlatBufferN::NTScalarArray<T>); \
				}
			MA(         char  )
			MA(unsigned char  )
			MA(         short )
			MA(unsigned short )
			MA(         int   )
			MA(unsigned int   )
			MA(         long  )
			MA(unsigned long  )
			MA(         float )
			MA(         double)
			#undef MA
		}
		throw std::runtime_error("ERROR can not convert this NTScalar");
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
	LOG(3, "Message for: %s  msg: %s  msgtype: %s", getRequesterName().c_str(), message.c_str(), getMessageTypeName(messageType).c_str());
}



/*
Seems that channel creation is actually a synchronous operation
and that this requester callback is called from the same stack
from which the channel creation was initiated.
*/

void ChannelRequester::channelCreated(epics::pvData::Status const & status, Channel::shared_pointer const & channel) {
	auto monitor = action->monitor.lock();
	#if TEST_RANDOM_FAILURES
		std::mt19937 rnd(std::chrono::system_clock::now().time_since_epoch().count());
		if (rnd() < 0.1 * 0xffffffff) {
			if (monitor) monitor->go_into_failure_mode();
		}
	#endif
	if (!status.isSuccess()) {
		// quick fix until decided on logging system..
		std::ostringstream s1;
		s1 << status;
		LOG(6, "failure: %s", s1.str().c_str());
		if (channel) {
			// Yes, take a copy
			std::string cname = channel->getChannelName();
			LOG(6, "  failure is in channel: %s", cname.c_str());
		}
		if (monitor) monitor->go_into_failure_mode();
	}
	LOG(0, "success (%d)", (int)status.isOK());
	if (!status.isOK()) {
		// quick fix until decided on logging system..
		std::ostringstream s1;
		s1 << status;
		LOG(3, "WARNING %s", s1.str().c_str());
	}
}

void ChannelRequester::channelStateChange(Channel::shared_pointer const & channel, Channel::ConnectionState cstate) {
	auto monitor = action->monitor.lock();
	LOG(0, "channel state change: %s", Channel::ConnectionStateNames[cstate]);
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
		LOG(6, "Unhandled channel state change: %s", channel_state_name(cstate));
		if (monitor) monitor->go_into_failure_mode();
	}
	if (!channel) {
		LOG(6, "ERROR no channel, even though we should have.  state: %s", channel_state_name(cstate));
		if (monitor) monitor->go_into_failure_mode();
	}

	action->operator()(channel);

		#if 0
		if (true) {
			// Initiate a single channel get

			// TODO why not just pass the channel if we have it anyway?
			// And what do we use ChannelGetRequesterDW for anyway???
			cgr.reset(new ChannelGetRequesterDW());
			//ChannelGetRequesterDW * cgri = dynamic_cast<ChannelGetRequesterDW*>(cgr.get());

			// Is this some magic string?!?!??
			// Yes.  Not yet sure about how it works.
			// If I specify 'value' I get the same as with an empty string.
			// A null errors.
			// 'alarm' gives me the alarm structure, and my inspect routine sees it.
			// But how do I inspect everything in that channel if I do not know that e.g. 'alarm' exists???

			// The channel->getField() can do that, see above.  That will return the full
			// introspection data and I can see all the primary fields of the channel.

			// Is there no way to do a wildcard request at this point ??

			string request = "value,timeStamp";
			PVStructure::shared_pointer pvreq;
			pvreq = epics::pvData::CreateRequest::create()->createRequest(request);
			if (pvreq == nullptr) {
				LOG0("can not create a pv request");
				throw exception();
			}

			// Must keep the returned channel get instance alive
			cg = channel->createChannelGet(cgr, pvreq);
		}


		if (false) {
			// Initiate a monitor on the channel
			monr.reset(new MonitorRequester);
			string request = "";
			PVStructure::shared_pointer pvreq;
			pvreq = epics::pvData::CreateRequest::create()->createRequest(request);
			mon = channel->createMonitor(monr, pvreq);
		}
		#endif

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
	LOG(3, "%s", msg.c_str());
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
MonitorRequester(Monitor::wptr monitor_HL);
~MonitorRequester();
string getRequesterName() override;
void message(string const & msg, epics::pvData::MessageType msgT) override;

void monitorConnect(epics::pvData::Status const & status, epics::pvData::Monitor::shared_pointer const & monitor, epics::pvData::StructureConstPtr const & structure) override;

void monitorEvent(epics::pvData::MonitorPtr const & monitor) override;
void unlisten(epics::pvData::MonitorPtr const & monitor) override;

private:
//epics::pvData::MonitorPtr monitor;
Monitor::wptr monitor_HL;
PVStructureToFlatBuffer::ptr conv_to_flatbuffer;
};

MonitorRequester::MonitorRequester(Monitor::wptr monitor_HL) :
	monitor_HL(monitor_HL)
{
}


MonitorRequester::~MonitorRequester() {
	LOG(0, "dtor");
}

string MonitorRequester::getRequesterName() { return "MonitorRequester"; }

void MonitorRequester::message(string const & msg, epics::pvData::MessageType msgT) {
	LOG(3, "%s", msg.c_str());
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



// And again, the docs are basically non-existent.
// But according to the example, we must release the 'element' in the end.
// TODO
// Is it possible to miss events if we poll() only once per 'monitorEvent' ?

void MonitorRequester::monitorEvent(epics::pvData::MonitorPtr const & monitor) {
	LOG(0, "got event");

	auto monitor_HL = this->monitor_HL.lock();
	if (not monitor_HL) {
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
		if (false) inspect_PVField(*pvstr);

		// NOTE
		// One assumption is currently that we stream only certain types from EPICS,
		// including NTScalar and NTScalarArray.
		// This is implemneted currently by passing the PVStructure to this function which decides
		// based on the naming scheme what type it contains.
		// A more robust solution in the future should actually investigate the PVStructure.
		// Open question:  Could EPICS suddenly change the type during runtime?
		if (not conv_to_flatbuffer) conv_to_flatbuffer = PVStructureToFlatBuffer::create(pvstr);
		auto flat_buffer = conv_to_flatbuffer->convert(pvstr);
		monitor_HL->emit(std::move(flat_buffer));
		monitor->release(ele);
	}
}

void MonitorRequester::unlisten(epics::pvData::MonitorPtr const & monitor) {
	LOG(3, "monitor source no longer available");
}









class IntrospectField : public ActionOnField {
public:
IntrospectField(Monitor::wptr monitor) : ActionOnField(monitor) { }
void operator () (Field const & field) override {
	// TODO
	// Check that we understand the field content.
	// Need to discuss possible constraints?
	LOG(3, "inspecting field");
	inspect_Field(field);
}
};








class StartMonitorChannel : public ActionOnChannel {
public:
StartMonitorChannel(Monitor::wptr monitor) : ActionOnChannel(monitor) { }

void operator () (epics::pvAccess::Channel::shared_pointer const & channel) override {
	auto monitor = this->monitor.lock();
	if (monitor) {
		monitor->initiate_value_monitoring();
	}
	return;

	// The remainder is optional, used for testing:
	// It demonstrated the getField() on a channel, which requests a specific
	// subfield.

	// Channel::getField is used to get the introspection information.
	// It is not necessary if we are only interested in the value itself.
	gfr.reset(new GetFieldRequesterForAction(channel, std::unique_ptr<IntrospectField>(new IntrospectField(monitor) )));

	// 2nd parameter:
	// http://epics-pvdata.sourceforge.net/docbuild/pvAccessCPP/tip/documentation/html/classepics_1_1pvAccess_1_1Channel.html#a506309575eed705e97cf686bd04f5c04

	// 2nd argument does maybe specify the subfield, not tested yet.  NOTE: *sub*-field!
	// 'pvinfo' actually uses an empty string here.
	// Is empty string then 'the whole channel itself' ?  Apparently yes.
	// "" is the full field
	channel->getField(gfr, "");
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
			//LOG(5, "stopping monitor for TM %d", topic_mapping->id);

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
		LOG(5, "Runtime error from Epics: %s", e.what());
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
	// TODO
	// Does it do any harm to call this multiple times per thread/process?
	static bool do_init = true;
	//static epics::pvAccess::ChannelProviderRegistry::shared_pointer provider;
	if (do_init) {
		epics::pvAccess::ClientFactory::start();
		provider = epics::pvAccess::getChannelProviderRegistry()
			->getProvider("pva");
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
	monr.reset(new MonitorRequester(self));

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
