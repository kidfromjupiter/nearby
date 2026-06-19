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

#include "internal/platform/implementation/linux/ble_l2cap_socket.h"

#include <sys/socket.h>
#include <unistd.h>

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include "absl/time/time.h"
#include "internal/platform/implementation/crypto.h"
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

class BleL2capSocketTest : public ::testing::Test {
protected:
void SetUp() override {
    ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, fds_), 0);

    socket_fd_ = fds_[0];
    peer_fd_ = fds_[1];

    socket_ = std::make_unique<BleL2capSocket>(socket_fd_, 1);
  }

  void TearDown() override {
    if (socket_) {
      socket_->Close();
    }

    if (peer_fd_ >= 0) {
      close(peer_fd_);
    }
  }
  int fds_[2]{-1, -1};
  int socket_fd_{-1};
  int peer_fd_{-1};
  std::unique_ptr<BleL2capSocket> socket_;
};
TEST_F(BleL2capSocketTest, ReturnsInputAndOutputStreams) {
  nearby::InputStream& input = socket_->GetInputStream();
  nearby::OutputStream& output = socket_->GetOutputStream();

  EXPECT_NE(&input, nullptr);
  EXPECT_NE(&output, nullptr);
}
TEST_F(BleL2capSocketTest, ReturnsSameStreamInstancesAcrossCalls) {
  nearby::InputStream& input1 = socket_->GetInputStream();
  nearby::InputStream& input2 = socket_->GetInputStream();

  nearby::OutputStream& output1 = socket_->GetOutputStream();
  nearby::OutputStream& output2 = socket_->GetOutputStream();

  EXPECT_EQ(&input1, &input2);
  EXPECT_EQ(&output1, &output2);
}
TEST_F(BleL2capSocketTest, ReadReceivesExactBytesFromPeer) {
  std::string message = "hello into l2cap socket";
  std::string framed_message = WrapFrame(message);

  ASSERT_EQ(
      write(peer_fd_, framed_message.data(), framed_message.size()),
      static_cast<ssize_t>(framed_message.size())
  );

  nearby::InputStream& input = socket_->GetInputStream();

  std::vector<char> buffer(message.size());
  auto out = input.Read(buffer.size()).GetResult();

  EXPECT_EQ(
      out,
      ByteArray(message)
  );

  EXPECT_EQ(
      out.AsStringView(),
      message
  );
}
TEST_F(BleL2capSocketTest, WriteSendsExactBytesToPeer) {
  std::string message = "hello from l2cap socket";
  std::string framed_message = WrapFrame(message);

  nearby::OutputStream& output = socket_->GetOutputStream();

  EXPECT_EQ(output.Write(message).value, Exception::kSuccess);

  std::vector<char> received(framed_message.size());

  ssize_t n = read(peer_fd_, received.data(), received.size());

  ASSERT_EQ(n, static_cast<ssize_t>(framed_message.size()));

  EXPECT_EQ(
      std::string(received.begin(), received.end()),
      framed_message
  );
}
TEST_F(BleL2capSocketTest, SkipSkipsBytesFromPeer) {
  std::string message = "hello there";
  std::string framed_message = WrapFrame(message);

  ASSERT_EQ(
      write(peer_fd_, framed_message.data(), framed_message.size()),
      static_cast<ssize_t>(framed_message.size())
  );

  nearby::InputStream& input = socket_->GetInputStream();

  EXPECT_EQ(input.Skip(6).result(), 6);

  auto out = input.Read(5).GetResult();
  EXPECT_EQ(out, ByteArray("there"));
}
TEST_F(BleL2capSocketTest, CloseReturnsSuccess) {
  EXPECT_EQ(socket_->Close().value, Exception::kSuccess);
}

TEST(BleL2capSocketSeqpacketTest, ReadReceivesHeaderAndPayloadFromOnePacket) {
  int fds[2]{-1, -1};
  ASSERT_EQ(socketpair(AF_UNIX, SOCK_SEQPACKET, 0, fds), 0);

  auto socket = std::make_unique<BleL2capSocket>(fds[0], 1);
  int peer_fd = fds[1];

  std::string message = "\x03";
  std::string framed_message = WrapFrame(message);

  ASSERT_EQ(write(peer_fd, framed_message.data(), framed_message.size()),
            static_cast<ssize_t>(framed_message.size()));

  auto out = socket->GetInputStream().Read(1).GetResult();
  EXPECT_EQ(out, ByteArray(message));

  socket->Close();
  close(peer_fd);
}
}  // namespace
}  // namespace linux
}  // namespace nearby
