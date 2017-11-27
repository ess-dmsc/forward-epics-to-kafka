#include "helper.h"
#include <fstream>
#ifdef _MSC_VER
#include "wingetopt.h"
#else
#include <getopt.h>
#include <unistd.h>
#endif
#include "logger.h"
#include <array>
#include <chrono>
#include <fstream>
#include <rapidjson/document.h>
#include <string>
#include <thread>
#include <vector>

std::vector<char> gulp(std::string fname) {
  std::vector<char> ret;
  std::ifstream ifs(fname, std::ios::binary | std::ios::ate);
  if (!ifs.good())
    return ret;
  auto n1 = ifs.tellg();
  if (n1 <= 0)
    return ret;
  ret.resize(n1);
  ifs.seekg(0);
  ifs.read(ret.data(), n1);
  return ret;
}

std::vector<char> binary_to_hex(char const *data, int len) {
  std::vector<char> ret;
  ret.reserve(len * 5 / 4);
  for (int i1 = 0; i1 < len; ++i1) {
    auto c = (uint8_t)data[i1];
    for (auto &v :
         std::array<uint8_t, 2>{{(uint8_t)(c >> 4), (uint8_t)(c & 0x0f)}}) {
      if (v < 10)
        v += 48;
      else
        v += 97 - 10;
      ret.push_back(v);
    }
    if (i1 % 8 == 7)
      ret.push_back(' ');
    if (i1 % (4 * 8) == 4 * 8 - 1)
      ret.push_back('\n');
  }
  return ret;
}

std::vector<std::string> split(std::string const &input, std::string token) {
  using std::vector;
  using std::string;
  vector<string> ret;
  if (token.empty()) {
    if (input.empty()) {
      return {};
    }
    return {input};
  }
  string::size_type i1 = 0;
  while (true) {
    auto i2 = input.find(token, i1);
    if (i2 == string::npos)
      break;
    if (i2 > i1) {
      ret.push_back(input.substr(i1, i2 - i1));
    }
    i1 = i2 + 1;
  }
  if (i1 != input.size()) {
    ret.push_back(input.substr(i1));
  }
  return ret;
}

std::string get_string(rapidjson::Value const *v, std::string path) {
  auto a = split(path, ".");
  uint32_t i1 = 0;
  for (auto &x : a) {
    bool num = true;
    for (char &c : x) {
      if (c < 48 || c > 57) {
        num = false;
        break;
      }
    }
    if (num) {
      if (!v->IsArray())
        return "";
      auto n1 = (uint32_t)strtol(x.c_str(), nullptr, 10);
      if (n1 >= v->Size())
        return "";
      auto &v2 = v->GetArray()[n1];
      if (i1 == a.size() - 1) {
        if (v2.IsString()) {
          return v2.GetString();
        }
      } else {
        v = &v2;
      }
    } else {
      if (!v->IsObject())
        return "";
      auto it = v->FindMember(x.c_str());
      if (it == v->MemberEnd()) {
        return "";
      }
      if (i1 == a.size() - 1) {
        if (it->value.IsString()) {
          return it->value.GetString();
        }
      } else {
        v = &it->value;
      }
    }
    ++i1;
  }
  return "";
}

void sleep_ms(uint32_t ms) {
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}
