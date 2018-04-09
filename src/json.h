#pragma once

#include <nlohmann/json.hpp>
#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/schema.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <stdexcept>
#include <string>

class RapidjsonParseError : public std::runtime_error {
public:
  RapidjsonParseError(std::string msg) : std::runtime_error(msg) {}
};

rapidjson::Document stringToRapidjsonOrThrow(std::string const &JsonString);

std::string json_to_string(rapidjson::Value const &jd);

rapidjson::Document merge(rapidjson::Value const &v1,
                          rapidjson::Value const &v2);

template <typename T> class JsonMaybe {
public:
  JsonMaybe() {}
  JsonMaybe(T inner) : inner_(inner), found_(true) {}
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
