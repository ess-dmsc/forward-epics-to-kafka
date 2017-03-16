#include "helper.h"
#include <fstream>
#include <unistd.h>
#include <array>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>

std::vector<char> gulp(std::string fname) {
	std::vector<char> ret;
	std::ifstream ifs(fname, std::ios::binary | std::ios::ate);
	if (!ifs.good()) return ret;
	auto n1 = ifs.tellg();
	if (n1 <= 0) return ret;
	ret.resize(n1);
	ifs.seekg(0);
	ifs.read(ret.data(), n1);
	return ret;
}

std::vector<char> binary_to_hex(char const * data, int len) {
	std::vector<char> ret;
	ret.reserve(len * 5 / 4);
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

std::vector<std::string> split(std::string const & input, std::string token) {
	using std::vector;
	using std::string;
	vector<string> ret;
	if (token.size() == 0) return { input };
	string::size_type i1 = 0;
	while (true) {
		auto i2 = input.find(token, i1);
		if (i2 == string::npos) break;
		if (i2 > i1) {
			ret.push_back(input.substr(i1, i2-i1));
		}
		i1 = i2 + 1;
	}
	if (i1 != input.size()) {
		ret.push_back(input.substr(i1));
	}
	return ret;
}

#if HAVE_GTEST
#include <gtest/gtest.h>

TEST(helper, split_01) {
	using std::vector;
	using std::string;
	auto v = split("", "");
	ASSERT_TRUE(v == vector<string>({""}));
}

TEST(helper, split_02) {
	using std::vector;
	using std::string;
	auto v = split("abc", "");
	ASSERT_TRUE(v == vector<string>({"abc"}));
}

TEST(helper, split_03) {
	using std::vector;
	using std::string;
	auto v = split("a/b", "/");
	ASSERT_TRUE(v == vector<string>({"a", "b"}));
}

TEST(helper, split_04) {
	using std::vector;
	using std::string;
	auto v = split("/a/b", "/");
	ASSERT_TRUE(v == vector<string>({"a", "b"}));
}

TEST(helper, split_05) {
	using std::vector;
	using std::string;
	auto v = split("ac/dc/", "/");
	ASSERT_TRUE(v == vector<string>({"ac", "dc"}));
}

TEST(helper, split_06) {
	using std::vector;
	using std::string;
	auto v = split("/ac/dc/", "/");
	ASSERT_TRUE(v == vector<string>({"ac", "dc"}));
}

TEST(helper, split_07) {
	using std::vector;
	using std::string;
	auto v = split("/some/longer/thing/for/testing", "/");
	ASSERT_TRUE(v == vector<string>({"some", "longer", "thing", "for", "testing"}));
}

#endif

#include "logger.h"

std::string get_string(rapidjson::Value const * v, std::string path) {
	auto a = split(path, ".");
	int i1 = 0;
	for (auto & x : a) {
		bool num = true;
		for (char & c : x) {
			if (c < 48 || c > 57) { num = false; break; }
		}
		if (num) {
			if (!v->IsArray()) return "";
			int n1 = (int)strtol(x.c_str(), nullptr, 10);
			if (n1 >= v->Size()) return "";
			auto & v2 = v->GetArray()[n1];
			if (i1 == a.size() - 1) {
				if (v2.IsString()) {
					return v2.GetString();
				}
			}
			else {
				v = &v2;
			}
		}
		else {
			if (!v->IsObject()) return "";
			auto it = v->FindMember(x.c_str());
			if (it == v->MemberEnd()) {
				return "";
			}
			if (i1 == a.size() - 1) {
				if (it->value.IsString()) {
					return it->value.GetString();
				}
			}
			else {
				v = &it->value;
			}
		}
		++i1;
	}
	return "";
}

#if HAVE_GTEST
#include <gtest/gtest.h>

TEST(RapidTools, get_string_01) {
	using namespace rapidjson;
	Document d;
	d.SetObject();
	auto & a = d.GetAllocator();
	d.AddMember("mem00", Value("s1", a), a);
	Value v2;
	v2.SetObject();
	v2.AddMember("mem10", Value("s2", a), a);
	d.AddMember("mem01", v2.Move(), a);

	{
		Value va;
		va.SetArray();
		va.PushBack(Value("something_a_0", a), a);
		va.PushBack(Value("something_a_1", a), a);
		va.PushBack(Value(1234), a);
		d.AddMember("mem02", va, a);
	}

	StringBuffer buf1;
	PrettyWriter<StringBuffer> wr(buf1);
	d.Accept(wr);
	auto s1 = get_string(&d, "mem00");
	ASSERT_EQ(s1, "s1");
	s1 = get_string(&d, "mem01.mem10");
	ASSERT_EQ(s1, "s2");
	s1 = get_string(&d, "mem02.1");
	ASSERT_EQ(s1, "something_a_1");
}

#endif


void sleep_ms(uint32_t ms) {
	std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

#if HAVE_GTEST
#include <gtest/gtest.h>
TEST(Sleep, sleep_ms) {
	sleep_ms(1);
	// ;-)
}
#endif
