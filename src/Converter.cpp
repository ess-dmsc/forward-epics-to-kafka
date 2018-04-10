#include "Converter.h"
#include "json.h"
#include "logger.h"

namespace BrightnESS {
namespace ForwardEpicsToKafka {

Converter::sptr
Converter::create(FlatBufs::SchemaRegistry const &schema_registry,
                  std::string schema, MainOpt const &main_opt) {
  auto ret = Converter::sptr(new Converter);
  ret->schema = schema;
  auto r1 = schema_registry.items().find(schema);
  if (r1 == schema_registry.items().end()) {
    LOG(3, "can not handle (yet?) schema id {}", schema);
    return nullptr;
  }
  ret->conv = r1->second->create_converter();
  auto &conv = ret->conv;
  if (!conv) {
    LOG(3, "can not create a converter");
    return ret;
  }
  using std::string;
  using nlohmann::json;
  std::map<string, int64_t> config_ints;
  std::map<string, string> config_strings;

  auto const &JSON = main_opt.JSONConfiguration;
  if (JSON.is_object()) {
    if (auto x = find<json>("converters", JSON)) {
      auto const &Converters = x.inner();
      if (Converters.is_object()) {
        if (auto x = find<json>(schema, Converters)) {
          auto const &ConverterSchemaConfig = x.inner();
          if (ConverterSchemaConfig.is_object()) {
            for (auto SettingIt = ConverterSchemaConfig.begin();
                 SettingIt != ConverterSchemaConfig.end(); ++SettingIt) {
              if (SettingIt.value().is_number()) {
                config_ints[SettingIt.key()] = SettingIt.value().get<int64_t>();
              }
              if (SettingIt.value().is_string()) {
                config_strings[SettingIt.key()] =
                    SettingIt.value().get<string>();
              }
            }
          }
        }
      }
    }
  }
  conv->config(config_ints, config_strings);
  return ret;
}

BrightnESS::FlatBufs::FlatbufferMessage::uptr
Converter::convert(FlatBufs::EpicsPVUpdate const &up) {
  return conv->convert(up);
}

std::map<std::string, double> Converter::stats() { return conv->stats(); }

std::string Converter::schema_name() { return schema; }
}
}
