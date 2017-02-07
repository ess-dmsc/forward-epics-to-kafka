#include <cstdlib>
#include <cstdio>
#include <vector>
#include "helper.h"
#include "logger.h"
#include "fbhelper.h"
#include "fbschemas.h"

using namespace BrightnESS::ForwardEpicsToKafka::Epics;

#define STRINGIFY2(x) #x
#define STRINGIFY(x) STRINGIFY2(x)

template <typename T> char const * type_fmt();
//template <> char const * type_name<uint32_t>() { return "uint32_t"; }
// Unstringification not possible, so not possible to give white space..
#define M(x, y) template <> char const * type_fmt<x>() { return y; }
M( int8_t, "% d")
M(uint8_t, "%u")
M( int16_t, "% d")
M(uint16_t, "%u")
M( int32_t, "% d")
M(uint32_t, "%u")
M( int64_t, "% ld")
M(uint64_t, "%lu")
M( float, "% e")
M(double, "% e")
#undef M




template <typename T0>
void print_array(EpicsPV const * b1) {
	using namespace std;
	auto pv = static_cast<T0 const *>(b1->pv());
	auto a1 = pv->value();
	char const N1 = 20;
	static char fmt[N1] = { 0 };
	if (fmt[0] == 0) {
		using T1 = typename std::remove_pointer<decltype(std::declval<T0>().value())>::type::return_type;
		snprintf(fmt, N1, "a[%%3d] = %s\n", type_fmt<T1>());
	}
	for (size_t i1 = 0; i1 < a1->Length(); ++i1) {
		printf(fmt, i1, a1->Get(i1));
	}
}


std::vector<char> gulp(FILE * f1) {
	std::vector<char> ret;
	uint32_t cap = 4096;
	uint32_t c1 = 0;
	ret.reserve(cap);
	while (true) {
		auto d1 = cap - c1;
		auto n1 = fread(ret.data() + c1, 1, d1, f1);
		if (n1 > 0) {
			LOG(5, "read n1 {}", n1);
		}
		else {
			if (feof(f1)) {
				break;
			}
			if (ferror(f1)) {
				LOG(5, "error on file read");
				break;
			}
		}
		if (2*c1/cap >= 1) {
			LOG(5, "re");
			cap = 4 * cap / 3;
			ret.reserve(cap);
		}
	}
	return ret;
}


int main(int argc, char ** argv) {
	using namespace BrightnESS::ForwardEpicsToKafka::Epics;
	using BrightnESS::ForwardEpicsToKafka::Epics::PV;
	auto buf1 = gulp(stdin);
	//FILE * f1 = fopen("buf1", "rb");
	//FILE * f1 = stdin;
	//std::vector<char> buf1;
	//std::vector<char> buf2;
	//fclose(f1);
	auto hex = binary_to_hex(buf1.data(), buf1.size());
	LOG(5, "buf1: {:.{}}", hex.data(), hex.size());
	auto b1 = GetEpicsPV(buf1.data());
	if (b1->pv_type() == PV::NTScalarShort) {
		auto pv = static_cast<NTScalarShort const *>(b1->pv());
		short sv = pv->value();
		LOG(5, "short [{}] value: {}", sizeof(sv), sv);
		auto hex = binary_to_hex((char*)&sv, sizeof(sv));
		LOG(5, "short binary: {:.{}}", hex.data(), hex.size());
	}
	if (b1->pv_type() == PV::NTScalarDouble) {
		auto pv = static_cast<NTScalarDouble const *>(b1->pv());
		LOG(5, "double value: {}", pv->value());
	}
	if (b1->pv_type() == PV::NTScalarFloat) {
		auto pv = static_cast<NTScalarFloat const *>(b1->pv());
		LOG(5, "float value: {}", pv->value());
	}
	if (b1->pv_type() == PV::NTScalarArrayDouble) {
		print_array<NTScalarArrayDouble>(b1);
	}
	if (b1->pv_type() == PV::NTScalarArrayFloat) {
		print_array<NTScalarArrayFloat>(b1);
	}
	if (b1->pv_type() == PV::NTScalarArrayInt) {
		print_array<NTScalarArrayInt>(b1);
	}
	return 0;
}
