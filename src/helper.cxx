#include <array>

#include "helper.h"

std::vector<char> binary_to_hex(char const * data, int len) {
	std::vector<char> ret;
	for (int i1 = 0; i1 < len; ++i1) {
		auto c = (uint8_t)data[i1];
		for (auto & v : std::array<uint8_t, 2>{ {(uint8_t)(c >> 4), (uint8_t)(c & 0x0f)} }) {
			if (v < 10) v += 48;
			else v += 97 - 10;
			ret.push_back(v);
		}
		if (i1 %    8  == 7) ret.push_back(' ');
		if (i1 % (4*8) == 4*8-1) ret.push_back('\n');
	}
	return ret;
}
