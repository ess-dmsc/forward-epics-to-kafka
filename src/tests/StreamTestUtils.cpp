// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

#include "StreamTestUtils.h"
#include <EpicsClient/EpicsClientRandom.h>

using namespace Forwarder;

std::shared_ptr<Stream> createStream(std::string ProviderType,
                                     std::string ChannelName) {
  auto ring = std::make_shared<
      moodycamel::ConcurrentQueue<std::shared_ptr<FlatBufs::EpicsPVUpdate>>>();
  auto client = std::make_unique<FakeEpicsClient>();
  ChannelInfo ci{std::move(ProviderType), std::move(ChannelName)};
  return std::make_shared<Stream>(ci, std::move(client), ring);
}

std::shared_ptr<Stream> createStreamRandom(std::string ProviderType,
                                           std::string ChannelName) {
  ChannelInfo ci{std::move(ProviderType), std::move(ChannelName)};
  auto ring = std::make_shared<
      moodycamel::ConcurrentQueue<std::shared_ptr<FlatBufs::EpicsPVUpdate>>>();
  auto client = std::make_unique<EpicsClient::EpicsClientRandom>(ci, ring);
  return std::make_shared<Stream>(ci, std::move(client), ring);
}
