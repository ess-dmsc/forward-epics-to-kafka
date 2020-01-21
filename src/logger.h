// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

#pragma once

// explicitly include winsock2, otherwise something below pulls in
// winsock and then clashes with winsock2 in asio
#ifdef _MSC_VER
#include <WinSock2.h>
#endif

#include <fmt/format.h>
#include <spdlog/spdlog.h>
#include <string>
#define UNUSED_ARG(x) (void)x;

using SharedLogger = std::shared_ptr<spdlog::logger>;

SharedLogger getLogger();

void setUpLogging(const spdlog::level::level_enum &LoggingLevel,
                  const std::string &LogFile, const std::string &GraylogURI);
