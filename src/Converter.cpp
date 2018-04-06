#include "Converter.h"
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
  using std::map;
  using std::string;
  map<string, int64_t> config_ints;
  map<string, string> config_strings;
  if (true) {
    if (main_opt.json) {
      auto m = main_opt.json->FindMember("converters");
      if (m != main_opt.json->MemberEnd()) {
        if (m->value.IsObject()) {
          auto m2 = m->value.GetObject().FindMember(schema.c_str());
          if (m2 != main_opt.json->MemberEnd()) {
            if (m2->value.IsObject()) {
              for (auto &v : m2->value.GetObject()) {
                if (v.value.IsInt64()) {
                  config_ints[v.name.GetString()] = v.value.GetInt64();
                }
                if (v.value.IsString()) {
                  config_strings[v.name.GetString()] = v.value.GetString();
                }
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

BrightnESS::FlatBufs::FB_uptr
Converter::convert(FlatBufs::EpicsPVUpdate const &up) {
  return conv->convert(up);
}

std::map<std::string, double> Converter::stats() { return conv->stats(); }

std::string Converter::schema_name() { return schema; }
}
}
