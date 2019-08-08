// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

#pragma once
#include <Stream.h>

class FakeEpicsClient : public Forwarder::EpicsClient::EpicsClientInterface {
public:
  FakeEpicsClient() = default;
  void emit(std::shared_ptr<FlatBufs::EpicsPVUpdate> /* Update */) override {}
  int stop() override {
    IsStopped = true;
    return 0;
  }
  void errorInEpics() override { status_ = -1; }
  int status() override { return status_; }
  bool IsStopped{false};
  std::string getConnectionState() override { return "FakeEpicsClient"; }

private:
  int status_{0};
};

std::shared_ptr<Forwarder::Stream> createStream(std::string ProviderType,
                                                std::string ChannelName);

std::shared_ptr<Forwarder::Stream> createStreamRandom(std::string ProviderType,
                                                      std::string ChannelName);
