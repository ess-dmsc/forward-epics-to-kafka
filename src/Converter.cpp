// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

#include "Converter.h"
#include "logger.h"

namespace Forwarder {

std::shared_ptr<Converter> Converter::create(FlatBufs::SchemaRegistry const &,
                                             std::string schema,
                                             MainOpt const &main_opt) {
  SharedLogger Logger = getLogger();
  auto ret = std::make_shared<Converter>();
  ret->schema = schema;
  auto r1 = FlatBufs::SchemaRegistry::items().find(schema);
  if (r1 == FlatBufs::SchemaRegistry::items().end()) {
    Logger->error("can not handle (yet?) schema id {}", schema);
    return nullptr;
  }
  ret->conv = r1->second->createConverter();
  auto &conv = ret->conv;
  if (!conv) {
    Logger->error("can not create a converter");
    return ret;
  }

  auto It = main_opt.MainSettings.GlobalConverters.find(schema);
  if (It != main_opt.MainSettings.GlobalConverters.end()) {
    auto GlobalConv = main_opt.MainSettings.GlobalConverters.at(schema);
    conv->config(GlobalConv);
  }

  return ret;
}

std::unique_ptr<FlatBufs::FlatbufferMessage>
Converter::convert(FlatBufs::EpicsPVUpdate const &up) {
  return conv->create(up);
}

std::map<std::string, double> Converter::stats() { return conv->getStats(); }

std::string Converter::schema_name() const { return schema; }
} // namespace Forwarder
