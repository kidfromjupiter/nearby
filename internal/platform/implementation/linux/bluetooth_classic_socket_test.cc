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

#include "internal/platform/implementation/linux/bluetooth_classic_socket.h"

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
class BluetoothClassicSocketTest : public ::testing::Test {
protected:
void SetUp() override {
    ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, fds_), 0);

    socket_fd_ = fds_[0];
    peer_fd_ = fds_[1];


    socket_ = std::make_unique<BluetoothSocket>(nullptr,sdbus::UnixFd(socket_fd_));
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
  std::unique_ptr<BluetoothSocket> socket_;
};
TEST_F(BluetoothClassicSocketTest, ReturnsInputAndOutputStreams) {
  InputStream& input = socket_->GetInputStream();
  OutputStream& output = socket_->GetOutputStream();

  EXPECT_NE(&input, nullptr);
  EXPECT_NE(&output, nullptr);
}
TEST_F(BluetoothClassicSocketTest, ReturnsSameStreamInstancesAcrossCalls) {
  InputStream& input1 = socket_->GetInputStream();
  InputStream& input2 = socket_->GetInputStream();

  OutputStream& output1 = socket_->GetOutputStream();
  OutputStream& output2 = socket_->GetOutputStream();

  EXPECT_EQ(&input1, &input2);
  EXPECT_EQ(&output1, &output2);
}
TEST_F(BluetoothClassicSocketTest, ReadReceivesExactBytesFromPeer) {
  std::string message = "hello into l2cap socket";

  ASSERT_EQ(
      write(peer_fd_, message.data(), message.size()),
      static_cast<ssize_t>(message.size())
  );

  InputStream& input = socket_->GetInputStream();

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
TEST_F(BluetoothClassicSocketTest, WriteSendsExactBytesToPeer) {
  std::string message = "hello from l2cap socket";

  OutputStream& output = socket_->GetOutputStream();

  EXPECT_EQ(output.Write(message).value, Exception::kSuccess);

  std::vector<char> received(message.size());

  ssize_t n = read(peer_fd_, received.data(), received.size());

  ASSERT_EQ(n, static_cast<ssize_t>(message.size()));

  EXPECT_EQ(
      std::string(received.begin(), received.end()),
      message
  );
}
TEST_F(BluetoothClassicSocketTest, SkipSkipsBytesFromPeer) {
  std::string message = "hello there";

  ASSERT_EQ(
      write(peer_fd_, message.data(), message.size()),
      static_cast<ssize_t>(message.size())
  );

  InputStream& input = socket_->GetInputStream();

  EXPECT_EQ(input.Skip(6).result(), 6);

  auto out = input.Read(5).GetResult();
  EXPECT_EQ(out, ByteArray("there"));
}
TEST_F(BluetoothClassicSocketTest, CloseReturnsSuccess) {
  EXPECT_EQ(socket_->Close().value, Exception::kSuccess);
}
}  // namespace
}  // namespace linux
}  // namespace nearby
