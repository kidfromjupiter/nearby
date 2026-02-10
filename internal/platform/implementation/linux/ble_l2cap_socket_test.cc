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
#include "proto/mediums/ble_frames.pb.h"

namespace nearby {
namespace linux {
namespace {

using ::location::nearby::mediums::SocketControlFrame;
using ::location::nearby::mediums::SocketVersion;

constexpr uint8_t kRequestDataConnection = 3;
constexpr uint8_t kResponseDataConnectionReady = 23;

class SocketPair final {
 public:
  SocketPair() {
    int fds[2] = {-1, -1};
    ASSERT_EQ(0, socketpair(AF_UNIX, SOCK_STREAM, 0, fds));
    left_ = fds[0];
    right_ = fds[1];
  }

  ~SocketPair() {
    if (left_ >= 0) close(left_);
    if (right_ >= 0) close(right_);
  }

  int left() const { return left_; }
  int right() const { return right_; }

 private:
  int left_ = -1;
  int right_ = -1;
};

bool ReadExact(int fd, char* data, size_t length) {
  size_t offset = 0;
  while (offset < length) {
    ssize_t read_count = recv(fd, data + offset, length - offset, 0);
    if (read_count <= 0) {
      return false;
    }
    offset += static_cast<size_t>(read_count);
  }
  return true;
}

std::optional<std::string> ReceiveFrame(int fd) {
  std::array<char, 4> header;
  if (!ReadExact(fd, header.data(), header.size())) {
    return std::nullopt;
  }
  const auto* bytes = reinterpret_cast<const uint8_t*>(header.data());
  uint32_t payload_size = (static_cast<uint32_t>(bytes[0]) << 24) |
                          (static_cast<uint32_t>(bytes[1]) << 16) |
                          (static_cast<uint32_t>(bytes[2]) << 8) |
                          static_cast<uint32_t>(bytes[3]);
  std::string payload(payload_size, '\0');
  if (payload_size > 0 && !ReadExact(fd, payload.data(), payload.size())) {
    return std::nullopt;
  }
  return payload;
}

bool SendFrame(int fd, const std::string& payload) {
  std::array<char, 4> header = {
      static_cast<char>((payload.size() >> 24) & 0xFF),
      static_cast<char>((payload.size() >> 16) & 0xFF),
      static_cast<char>((payload.size() >> 8) & 0xFF),
      static_cast<char>(payload.size() & 0xFF),
  };
  if (send(fd, header.data(), header.size(), 0) != static_cast<ssize_t>(header.size())) {
    return false;
  }
  if (!payload.empty() &&
      send(fd, payload.data(), payload.size(), 0) != static_cast<ssize_t>(payload.size())) {
    return false;
  }
  return true;
}

ByteArray ServiceHash(absl::string_view service_id) {
  ByteArray full_hash = Crypto::Sha256(service_id);
  EXPECT_GE(full_hash.size(), 3);
  return ByteArray(full_hash.data(), 3);
}

std::string BuildLegacyControlPacket(uint8_t command) {
  return std::string(1, static_cast<char>(command));
}

std::optional<uint8_t> ParseLegacyControlCommand(absl::string_view payload) {
  if (payload.empty()) return std::nullopt;
  if (payload.size() != 1 && payload.size() < 3) return std::nullopt;
  return static_cast<uint8_t>(payload[0]);
}

std::string BuildLegacyIntroPacket(const ByteArray& service_hash) {
  SocketControlFrame frame;
  frame.set_type(SocketControlFrame::INTRODUCTION);
  auto* intro = frame.mutable_introduction();
  intro->set_service_id_hash(service_hash.AsStringView());
  intro->set_socket_version(SocketVersion::V2);

  std::string serialized(frame.ByteSizeLong(), '\0');
  EXPECT_TRUE(frame.SerializeToArray(serialized.data(), serialized.size()));

  std::string packet("\x00\x00\x00", 3);
  packet.append(serialized);
  return packet;
}

bool IsLegacyIntroPacketForServiceHash(absl::string_view payload,
                                       const ByteArray& service_hash) {
  if (payload.size() <= 3 || payload.substr(0, 3) != "\x00\x00\x00") return false;

  SocketControlFrame frame;
  if (!frame.ParseFromArray(payload.data() + 3, payload.size() - 3)) return false;
  return frame.type() == SocketControlFrame::INTRODUCTION &&
         frame.has_introduction() &&
         frame.introduction().socket_version() == SocketVersion::V2 &&
         frame.introduction().service_id_hash() == service_hash.AsStringView();
}

TEST(BleL2capSocketTest, RefactoredOutputStreamWritesFramedPayload) {
  SocketPair pair;
  BleL2capSocket socket(pair.left(), /*peripheral_id=*/1);

  ASSERT_TRUE(socket.GetOutputStream().Write(ByteArray("hello")).Ok());
  auto payload = ReceiveFrame(pair.right());
  ASSERT_TRUE(payload.has_value());
  EXPECT_EQ(*payload, "hello");
}

TEST(BleL2capSocketTest, RefactoredInputStreamReadsFramedPayload) {
  SocketPair pair;
  BleL2capSocket socket(pair.left(), /*peripheral_id=*/1);

  ASSERT_TRUE(SendFrame(pair.right(), "world"));
  ExceptionOr<ByteArray> read_result = socket.GetInputStream().Read(5);
  ASSERT_TRUE(read_result.ok());
  EXPECT_EQ(read_result.result().string_data(), "world");
}

TEST(BleL2capSocketTest, LegacyWritePrefixesServiceHash) {
  SocketPair pair;
  ByteArray service_hash = ServiceHash("service");
  BleL2capSocket socket(pair.left(), /*peripheral_id=*/1,
                        BleL2capSocket::ProtocolMode::kLegacy,
                        /*service_id=*/"service", /*incoming_connection=*/false);

  ASSERT_TRUE(socket.GetOutputStream().Write(ByteArray("abc")).Ok());
  auto payload = ReceiveFrame(pair.right());
  ASSERT_TRUE(payload.has_value());
  ASSERT_EQ(payload->size(), service_hash.size() + 3);
  EXPECT_EQ(payload->substr(0, service_hash.size()), service_hash.AsStringView());
  EXPECT_EQ(payload->substr(service_hash.size()), "abc");
}

TEST(BleL2capSocketTest, LegacyOutgoingHandshakeSucceeds) {
  SocketPair pair;
  ByteArray service_hash = ServiceHash("service");
  BleL2capSocket socket(pair.left(), /*peripheral_id=*/1,
                        BleL2capSocket::ProtocolMode::kLegacy,
                        /*service_id=*/"service", /*incoming_connection=*/false);

  std::thread peer([&]() {
    auto request = ReceiveFrame(pair.right());
    ASSERT_TRUE(request.has_value());
    auto request_command = ParseLegacyControlCommand(*request);
    ASSERT_TRUE(request_command.has_value());
    EXPECT_EQ(*request_command, kRequestDataConnection);

    ASSERT_TRUE(SendFrame(pair.right(), BuildLegacyControlPacket(kResponseDataConnectionReady)));

    auto intro = ReceiveFrame(pair.right());
    ASSERT_TRUE(intro.has_value());
    EXPECT_TRUE(IsLegacyIntroPacketForServiceHash(*intro, service_hash));
  });

  EXPECT_TRUE(socket.PerformLegacyOutgoingHandshake(absl::Seconds(2)));
  peer.join();
}

TEST(BleL2capSocketTest, LegacyOutgoingHandshakeTimesOutWithoutResponse) {
  SocketPair pair;
  BleL2capSocket socket(pair.left(), /*peripheral_id=*/1,
                        BleL2capSocket::ProtocolMode::kLegacy,
                        /*service_id=*/"service", /*incoming_connection=*/false);

  std::thread peer([&]() {
    auto request = ReceiveFrame(pair.right());
    ASSERT_TRUE(request.has_value());
    auto request_command = ParseLegacyControlCommand(*request);
    ASSERT_TRUE(request_command.has_value());
    EXPECT_EQ(*request_command, kRequestDataConnection);
  });

  EXPECT_FALSE(socket.PerformLegacyOutgoingHandshake(absl::Milliseconds(200)));
  peer.join();
}

TEST(BleL2capSocketTest, LegacyIncomingConnectionHandlesHandshakeAndData) {
  SocketPair pair;
  ByteArray service_hash = ServiceHash("service");
  BleL2capSocket socket(pair.left(), /*peripheral_id=*/1,
                        BleL2capSocket::ProtocolMode::kLegacy,
                        /*service_id=*/"service", /*incoming_connection=*/true);

  std::thread peer([&]() {
    ASSERT_TRUE(SendFrame(pair.right(), BuildLegacyControlPacket(kRequestDataConnection)));

    auto response = ReceiveFrame(pair.right());
    ASSERT_TRUE(response.has_value());
    auto response_command = ParseLegacyControlCommand(*response);
    ASSERT_TRUE(response_command.has_value());
    EXPECT_EQ(*response_command, kResponseDataConnectionReady);

    ASSERT_TRUE(SendFrame(pair.right(), BuildLegacyIntroPacket(service_hash)));

    std::string data_payload(service_hash.AsStringView());
    data_payload.append("hello");
    ASSERT_TRUE(SendFrame(pair.right(), data_payload));
  });

  ExceptionOr<ByteArray> read_result = socket.GetInputStream().Read(5);
  ASSERT_TRUE(read_result.ok());
  EXPECT_EQ(read_result.result().string_data(), "hello");
  peer.join();
}

}  // namespace
}  // namespace linux
}  // namespace nearby
