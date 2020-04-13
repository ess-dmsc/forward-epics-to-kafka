// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

#pragma once

#include "ConfigParser.h"
#include "KafkaW/KafkaW.h"
#include "SchemaRegistry.h"
#include "URI.h"
#include <memory>
#include <string>
#include <vector>

namespace Forwarder {

struct MainOpt {
  ConfigSettings MainSettings;
  bool PrintVersion = false;
  std::string KafkaGELFAddress = "";
  spdlog::level::level_enum LogLevel;
  std::string GraylogLoggerAddress = "";
  std::string InfluxURI = "";
  std::string LogFilename;
  uint32_t PeriodMS = 0;
  uint32_t FakePVPeriodMS = 0;
  FlatBufs::SchemaRegistry schema_registry;
  KafkaW::BrokerSettings GlobalBrokerSettings;
};

enum class ParseOptRet { Success, VersionRequested, Error };

std::pair<ParseOptRet, std::unique_ptr<MainOpt>> parse_opt(int argc,
                                                           char **argv);

} // namespace Forwarder
