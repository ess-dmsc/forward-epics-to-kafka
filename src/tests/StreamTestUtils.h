#pragma once
#include "../Stream.h"

class FakeEpicsClient : public Forwarder::EpicsClient::EpicsClientInterface {
public:
  FakeEpicsClient() = default;
  int emit(std::shared_ptr<FlatBufs::EpicsPVUpdate> /* Update */) override {
    return 0;
  }
  int stop() override {
    IsStopped = true;
    return 0;
  }
  void errorInEpics() override { status_ = -1; }
  int status() override { return status_; }
  bool IsStopped{false};

private:
  int status_{0};
};

std::shared_ptr<Forwarder::Stream> createStream(std::string ProviderType,
                                                std::string ChannelName);

std::shared_ptr<Forwarder::Stream> createStreamRandom(std::string ProviderType,
                                                      std::string ChannelName);
