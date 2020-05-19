// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

#pragma once

#include <string>
#include <vector>

struct ConversionPathStatus {
  ConversionPathStatus(std::string const &Schema, std::string const &Broker,
                       std::string const &Topic)
      : Schema(Schema), Broker(Broker), Topic(Topic) {}

  std::string Schema;
  std::string Broker;
  std::string Topic;
};

struct StreamStatus {
  StreamStatus(std::string const &ChannelName,
               std::string const &ConnectionStatus)
      : ChannelName(ChannelName), ConnectionStatus(ConnectionStatus) {}
  std::string ChannelName;
  std::string ConnectionStatus;
  std::vector<ConversionPathStatus> Converters;
};
