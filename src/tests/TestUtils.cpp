#include "TestUtils.h"
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
