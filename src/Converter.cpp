#include "Converter.h"
#include "logger.h"

namespace Forwarder {

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

  auto It = main_opt.MainSettings.GlobalConverters.find(schema);
  if (It != main_opt.MainSettings.GlobalConverters.end()) {
    auto GlobalConv = main_opt.MainSettings.GlobalConverters.at(schema);
    conv->config(GlobalConv.ConfigurationIntegers,
                 GlobalConv.ConfigurationStrings);
  }

  return ret;
}

FlatBufs::FlatbufferMessage::uptr
Converter::convert(FlatBufs::EpicsPVUpdate const &up) {
  return conv->convert(up);
}

std::map<std::string, double> Converter::stats() { return conv->stats(); }

std::string Converter::schema_name() const { return schema; }
} // namespace Forwarder
