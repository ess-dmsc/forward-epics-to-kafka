#include "Converter.h"
#include "logger.h"

namespace Forwarder {

    std::shared_ptr<Converter> Converter::create(FlatBufs::SchemaRegistry const &,
                                                 std::string Schema,
                                                 MainOpt const &Options) {
  auto ret = std::make_shared<Converter>();
        ConverterPtr->SchemaID = Schema;
  auto r1 = FlatBufs::SchemaRegistry::items().find(Schema);
  if (r1 == FlatBufs::SchemaRegistry::items().end()) {
      LOG(spdlog::level::err, "can not handle (yet?) schema id {}", Schema);
    return nullptr;
  }
        ConverterPtr->FlatBufCreator = SchemaInRegistry->second->createConverter();
        auto &Creator = ConverterPtr->FlatBufCreator;
  if (!Creator) {
      LOG(spdlog::level::err, "can not create a converter");    return ret;
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
