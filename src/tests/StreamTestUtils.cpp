#include "StreamTestUtils.h"
#include "../EpicsClient/EpicsClientRandom.h"
#include "../helper.h"

using namespace Forwarder;

std::shared_ptr<Stream> createStream(std::string ProviderType,
                                     std::string ChannelName) {
  auto ring = std::make_shared<
      moodycamel::ConcurrentQueue<std::shared_ptr<FlatBufs::EpicsPVUpdate>>>();
  auto client = make_unique<FakeEpicsClient>();
  ChannelInfo ci{std::move(ProviderType), std::move(ChannelName)};
  return std::make_shared<Stream>(ci, std::move(client), ring);
}

std::shared_ptr<Stream> createStreamRandom(std::string ProviderType,
                                           std::string ChannelName) {
  ChannelInfo ci{std::move(ProviderType), std::move(ChannelName)};
  auto ring = std::make_shared<
      moodycamel::ConcurrentQueue<std::shared_ptr<FlatBufs::EpicsPVUpdate>>>();
  auto client = make_unique<EpicsClient::EpicsClientRandom>(ci, ring);
  return std::make_shared<Stream>(ci, std::move(client), ring);
}
