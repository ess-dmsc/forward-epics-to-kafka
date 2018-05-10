#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

// note: this implementation does not disable this overload for array types
template <typename T, typename... TX>
std::unique_ptr<T> make_unique(TX &&... tx) {
  return std::unique_ptr<T>(new T(std::forward<TX>(tx)...));
}

std::vector<char> readFile(std::string fname);

std::vector<char> binary_to_hex(char const *data, int len);

std::vector<std::string> split(std::string const &input, std::string token);

void sleep_ms(uint32_t ms);
