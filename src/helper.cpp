#include "helper.h"
#include "logger.h"
#include <array>
#include <chrono>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

/// Read a file into a vector as char buffer.
/// \param fname Filename to read.
/// \return Vector of chars from file.
std::vector<char> readFile(std::string fname) {
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
