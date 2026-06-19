// Copyright 2025 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "internal/platform/implementation/linux/ble_l2cap_connection.h"

#include <sys/socket.h>
#include <unistd.h>

#include <string>
#include <vector>

#include "internal/platform/byte_array.h"
#include "gtest/gtest.h"

namespace nearby {
namespace linux {
namespace {

std::string WrapFrame(absl::string_view payload) {
  return std::string{
      static_cast<char>((payload.size() >> 24) & 0xFF),
      static_cast<char>((payload.size() >> 16) & 0xFF),
      static_cast<char>((payload.size() >> 8) & 0xFF),
      static_cast<char>(payload.size() & 0xFF)} +
         std::string(payload);
}

class BleL2capConnectionTest : public ::testing::Test {
 protected:
  void SetUp() override {
    ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, fds_), 0);
    peer_fd_ = fds_[1];
    streams_ = CreateBleL2capConnectionStreams(fds_[0]);
  }

  void TearDown() override {
    if (streams_.input) streams_.input->Close();
    if (streams_.output) streams_.output->Close();
    if (peer_fd_ >= 0) close(peer_fd_);
  }

  int fds_[2]{-1, -1};
  int peer_fd_{-1};
  BleL2capConnectionStreams streams_;
};

TEST_F(BleL2capConnectionTest, ReadReceivesExactBytesFromPeer) {
  std::string message = "hello into l2cap socket";
  std::string framed_message = WrapFrame(message);

  ASSERT_EQ(write(peer_fd_, framed_message.data(), framed_message.size()),
            static_cast<ssize_t>(framed_message.size()));

  auto out = streams_.input->Read(message.size()).GetResult();
  EXPECT_EQ(out, ByteArray(message));
  EXPECT_EQ(out.AsStringView(), message);
}

TEST_F(BleL2capConnectionTest, WriteSendsExactBytesToPeer) {
  std::string message = "hello from l2cap socket";
  std::string framed_message = WrapFrame(message);

  EXPECT_EQ(streams_.output->Write(message).value, Exception::kSuccess);

  std::vector<char> received(framed_message.size());
  ssize_t n = read(peer_fd_, received.data(), received.size());

  ASSERT_EQ(n, static_cast<ssize_t>(framed_message.size()));
  EXPECT_EQ(std::string(received.begin(), received.end()), framed_message);
}

TEST_F(BleL2capConnectionTest, SkipSkipsBytesFromPeer) {
  std::string message = "hello there";
  std::string framed_message = WrapFrame(message);

  ASSERT_EQ(write(peer_fd_, framed_message.data(), framed_message.size()),
            static_cast<ssize_t>(framed_message.size()));

  EXPECT_EQ(streams_.input->Skip(6).result(), 6);

  auto out = streams_.input->Read(5).GetResult();
  EXPECT_EQ(out, ByteArray("there"));
}

}  // namespace
}  // namespace linux
}  // namespace nearby
