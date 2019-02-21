#include "Converter.h"
#include "logger.h"

namespace Forwarder {

std::shared_ptr<Converter> Converter::create(FlatBufs::SchemaRegistry const &,
                                             std::string Schema,
                                             MainOpt const &Options) {
  auto ConverterPtr = std::make_shared<Converter>();
  ConverterPtr->SchemaID = Schema;
  auto SchemaInRegistry = FlatBufs::SchemaRegistry::items().find(Schema);
  if (SchemaInRegistry == FlatBufs::SchemaRegistry::items().end()) {
    LOG(Sev::Error, "can not handle (yet?) schema id {}", Schema);
    return nullptr;
  }
  ConverterPtr->FlatBufCreator = SchemaInRegistry->second->createConverter();
  auto &Creator = ConverterPtr->FlatBufCreator;
  if (!Creator) {
    LOG(Sev::Error, "can not create a converter");
    return ConverterPtr;
  }

  if (Options.MainSettings.GlobalConverters.find(Schema) !=
      Options.MainSettings.GlobalConverters.end()) {
    auto GlobalConv = Options.MainSettings.GlobalConverters.at(Schema);
    Creator->config(GlobalConv);
  }

  return ConverterPtr;
}

std::unique_ptr<FlatBufs::FlatbufferMessage>
Converter::convert(FlatBufs::EpicsPVUpdate const &Update) {
  return FlatBufCreator->create(Update);
}

std::map<std::string, double> Converter::stats() {
  return FlatBufCreator->getStats();
}

std::string Converter::getSchemaID() const { return SchemaID; }
} // namespace Forwarder
