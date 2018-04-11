#pragma once

#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>

template <typename T> class JsonMaybe {
public:
  JsonMaybe() = default;
  explicit JsonMaybe(T inner) : inner_(inner), found_(true) {}
  explicit operator bool() const { return found_; }
  T inner() const {
    if (!found_) {
      throw std::runtime_error("Can not retrieve inner() on empty JsonMaybe");
    }
    return inner_;
  }

private:
  T inner_;
  bool found_ = false;
};

template <typename T>
JsonMaybe<T> find(std::string Key, nlohmann::json const &Json);

template <typename T>
JsonMaybe<T> find(std::string Key, nlohmann::json const &Json) {
  if (!Json.is_object()) {
    return JsonMaybe<T>();
  }
  auto It = Json.find(Key);
  if (It != Json.end()) {
    try {
      return JsonMaybe<T>(It.value().get<T>());
    } catch (...) {
      return JsonMaybe<T>();
    }
  }
  return JsonMaybe<T>();
}

JsonMaybe<nlohmann::json> find_array(std::string Key,
                                     nlohmann::json const &Json);
