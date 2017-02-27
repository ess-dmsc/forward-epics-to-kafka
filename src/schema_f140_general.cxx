#include "logger.h"
#include "epics-to-fb.h"
#include "schemas/f140_general_generated.h"

namespace BrightnESS {
namespace FlatBufs {
namespace f140 {



#include <fmt/format.h>
#if 0
#define FLOG(level, fmt, args...)  print("{:{}s}" fmt "\n", "", 2*(level), ## args);
#define DO_FLOG
#else
#define FLOG(level, fmt, args...)
#endif



namespace fbg {
	using std::vector;
	using fmt::print;
	//using F = FlatBufs::f140_general::F;
	using namespace FlatBufs::f140_general;
	//using ObjM = FlatBufs::f140_general::ObjM;
	typedef struct { F type; flatbuffers::Offset<void> off; } F_t;

	F_t Field(flatbuffers::FlatBufferBuilder & builder, epics::pvData::PVFieldPtr const & field, int level) {
		auto etype = field->getField()->getType();
		if (etype == epics::pvData::Type::structure) {
			auto pvstr = reinterpret_cast<epics::pvData::PVStructure*>(field.get());
			auto & subfields = pvstr->getPVFields();
			FLOG(level, "subfields.size(): {}", subfields.size());

			// For each subfield, collect the offsets:
			vector<F_t> fs;
			for (auto & f1ptr : subfields) {
				fs.push_back(Field(builder, f1ptr, 1+level));
			}

			#ifdef DO_FLOG
			for (auto & x : fs) {
				FLOG(level, "off: {:d}", x.off.o);
			}
			#endif

			// With the collected offsets, create object members
			// Collect raw vector of offsets to store later in flat buffer
			vector<flatbuffers::Offset<ObjM>> f2;
			for (auto & x : fs) {
				ObjMBuilder b1(builder);
				b1.add_v_type(x.type);
				b1.add_v(x.off);
				f2.push_back(b1.Finish());
			}
			auto v1 = builder.CreateVector(f2);

			ObjBuilder bo(builder);
			bo.add_ms(v1);
			return {F::Obj, bo.Finish().Union()};
		}

		else if (etype == epics::pvData::Type::structureArray) {
			// Serialize all objects, collect the offsets, and store an array of those.
			auto sa = reinterpret_cast<epics::pvData::PVValueArray<epics::pvData::PVStructurePtr> const *>(field.get());
			if (sa) {
				FLOG(level, "[size(): {}]", sa->view().size());
				vector<flatbuffers::Offset<Obj>> v1;
				for (auto & x : sa->view()) {
					FLOG(1+level, "OK");
					auto sub = Field(builder, x, 1+level);
					if (sub.type != F::Obj) {
						throw std::runtime_error("mismatched types in the EPICS structure");
						// TODO could return NONE?
					}
					v1.push_back(sub.off.o);
				}
				auto v2 = builder.CreateVector(v1);
				Obj_aBuilder b(builder);
				b.add_v(v2);
				// Normally, we should reach this return:
				return {F::Obj_a, b.Finish().Union()};
			}
			else {
				FLOG(level+2, "[ERROR could not dynamic_cast]");
			}
			return { F::NONE, 111333 };
		}

		else if (etype == epics::pvData::Type::scalar) {
			FLOG(level, "scalar");
			auto pvscalar = reinterpret_cast<epics::pvData::PVScalar const *>(field.get());
			auto stype = pvscalar->getScalar()->getScalarType();
			#define M(T, B, E) if (stype == epics::pvData::ScalarType::E) { \
				auto p1 = reinterpret_cast<epics::pvData::PVScalarValue<T> const *>(field.get()); \
				B b(builder); \
				b.add_v(p1->get()); \
				return {F::E, b.Finish().Union()}; \
			}
			M( int8_t,  pvByteBuilder,   pvByte);
			M( int16_t, pvShortBuilder,  pvShort);
			M( int32_t, pvIntBuilder,    pvInt);
			M( int64_t, pvLongBuilder,   pvLong);
			M(uint8_t,  pvUByteBuilder,  pvUByte);
			M(uint16_t, pvUShortBuilder, pvUShort);
			M(uint32_t, pvUIntBuilder,   pvUInt);
			M(uint64_t, pvULongBuilder,  pvULong);
			M(float,    pvFloatBuilder,  pvFloat);
			M(double,   pvDoubleBuilder, pvDouble);
			#undef M
			if (stype == epics::pvData::ScalarType::pvString) {
				auto p1 = reinterpret_cast<epics::pvData::PVScalarValue<std::string> const *>(field.get());
				auto s1 = builder.CreateString(p1->get());
				pvStringBuilder b(builder);
				b.add_v(s1);
				return { F::pvString, b.Finish().Union() };
			}
			return { F::NONE, 887700 };
		}

		else if (etype == epics::pvData::Type::scalarArray) {
			FLOG(level, "scalar array");
			auto pvSA = reinterpret_cast<epics::pvData::PVScalarArray const *>(field.get());
			auto stype = pvSA->getScalarArray()->getElementType();
			#define M(TC, TB, TF, TE) \
			if (stype == epics::pvData::ScalarType::TE) { \
				auto p1 = reinterpret_cast<epics::pvData::PVValueArray<TC> const *>(field.get()); \
				auto v1 = p1->view(); \
				auto v2 = builder.CreateVector(v1.data(), v1.size()); \
				TB b(builder); \
				b.add_v(v2); \
				return { F::TF, b.Finish().Union() }; \
			}
			M( int8_t,  pvByte_aBuilder,   pvByte_a,   pvByte);
			M( int16_t, pvShort_aBuilder,  pvShort_a,  pvShort);
			M( int32_t, pvInt_aBuilder,    pvInt_a,    pvInt);
			M( int64_t, pvLong_aBuilder,   pvLong_a,   pvLong);
			M(uint8_t,  pvUByte_aBuilder,  pvUByte_a,  pvUByte);
			M(uint16_t, pvUShort_aBuilder, pvUShort_a, pvUShort);
			M(uint32_t, pvUInt_aBuilder,   pvUInt_a,   pvUInt);
			M(uint64_t, pvULong_aBuilder,  pvULong_a,  pvULong);
			M(float,    pvFloat_aBuilder,  pvFloat_a,  pvFloat);
			M(double,   pvDouble_aBuilder, pvDouble_a, pvDouble);
			#undef M

			if (auto p1 = reinterpret_cast<epics::pvData::PVValueArray<std::string> const *>(field.get())) {
				vector<flatbuffers::Offset<flatbuffers::String>> v1;
				for (auto & s0 : p1->view()) {
					v1.push_back(builder.CreateString(s0));
				}
				auto v2 = builder.CreateVector(v1);
				pvString_aBuilder b(builder);
				b.add_v(v2);
				return { F::pvString_a, b.Finish().Union() };
			}
			throw std::runtime_error("is a type missing here?");
			return {F::NONE, 555};
		}

		else if (etype == epics::pvData::Type::union_) {
			FLOG(level, "union");
			auto f2 = reinterpret_cast<epics::pvData::PVUnion*>(field.get());
			if (f2) {
				auto f3 = f2->get();
				if (f3) {
					return Field(builder, f2->get(), 1+level);
				}
				else {
					// The union does not contain anything:
					return {F::NONE, 0};
				}
			}
			else {
				throw std::runtime_error("should never happen");
				// TODO we could ignore this and return NONE
			}
		}

		else if (etype == epics::pvData::Type::unionArray) {
			FLOG(level, "union array");
			throw std::runtime_error("union array not yet supported");
			return {F::NONE, 777};
		}

		else {
			throw std::runtime_error("Somethings wrong, none of the known types match");
		}
	}
}



std::vector<char> binary_to_hex(char const * data, int len) {
	std::vector<char> ret;
    ret.reserve(len*2);
	for (int i1 = 0; i1 < len; ++i1) {
		auto c = (uint8_t)data[i1];
        std::stringstream stream;
        stream << std::setfill ('0') << std::setw(2) << std::hex << int(c);
        ret.push_back(stream.str()[0]);
        ret.push_back(stream.str()[1]);
	}
	return ret;
}




class Converter : public MakeFlatBufferFromPVStructure {
public:
BrightnESS::FlatBufs::FB_uptr convert(EpicsPVUpdate const & up) override {
	//LOG(0, "conv_to_fb_general");
	// Passing initial size:
	auto fb = BrightnESS::FlatBufs::FB_uptr(new BrightnESS::FlatBufs::FB);
	uint64_t ts_data = 0;
	if (auto x = up.pvstr->getSubField<epics::pvData::PVScalarValue<uint64_t>>("ts")) {
		ts_data = x->get();
	}
	fb->seq = up.seq;
	fb->fwdix = up.fwdix;
	auto builder = fb->builder.get();
	auto n = builder->CreateString("some-name-must-go-here");
	auto vF = fbg::Field(*builder, up.pvstr, 0);
	//some kind of 'union F' offset:   flatbuffers::Offset<void>
	FlatBufs::f140_general::PVBuilder b(*builder);
	b.add_n(n);
	b.add_v_type(vF.type);
	auto fi = FlatBufs::f140_general::fwdinfo_t(up.seq, ts_data, up.ts_epics_monitor, up.fwdix);
	b.add_fwdinfo(&fi);
	//builder->Finish(b.Finish(), FlatBufs::f140_general::PVIdentifier());
	FinishPVBuffer(*builder, b.Finish());
	{
		auto b1 = binary_to_hex((char const *)builder->GetBufferPointer(), builder->GetSize());
		//auto b1 = binary_to_hex("f140", 4);
		LOG(9, "buffer: [{}] {:.{}}", FlatBufs::f140_general::PVIdentifier(), b1.data(), b1.size());
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


FlatBufs::SchemaRegistry::Registrar<Info> g_registrar_info("f140", std::move(Info::ptr(new Info)));






}
}
}
