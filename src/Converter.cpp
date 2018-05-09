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

  main_opt.Config->extractGlobalConverters(schema);
  conv->config(main_opt.Config->ConverterInts, main_opt.Config->ConverterStrings);

  return ret;
}

BrightnESS::FlatBufs::FlatbufferMessage::uptr
Converter::convert(FlatBufs::EpicsPVUpdate const &up) {
  return conv->convert(up);
}

std::map<std::string, double> Converter::stats() { return conv->stats(); }

std::string Converter::schema_name() { return schema; }
} // namespace ForwardEpicsToKafka
} // namespace BrightnESS
