// Copyright 2024 Google LLC
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

#include <algorithm>
#include <cstdint>
#include <limits>
#include <string>

#include "absl/strings/escaping.h"
#include "absl/strings/string_view.h"
#include "internal/platform/logging.h"

namespace nearby {
namespace linux {

namespace {

constexpr int kFrameHeaderSize = 4;
constexpr int kMaxFrameSize = 1024 * 1024;

std::string EncodeFrameLength(uint32_t length) {
  return std::string{
      static_cast<char>((length >> 24) & 0xFF),
      static_cast<char>((length >> 16) & 0xFF),
      static_cast<char>((length >> 8) & 0xFF),
      static_cast<char>(length & 0xFF),
  };
}

uint32_t DecodeFrameLength(absl::string_view header) {
  return (static_cast<uint32_t>(static_cast<unsigned char>(header[0])) << 24) |
         (static_cast<uint32_t>(static_cast<unsigned char>(header[1])) << 16) |
         (static_cast<uint32_t>(static_cast<unsigned char>(header[2])) << 8) |
         static_cast<uint32_t>(static_cast<unsigned char>(header[3]));
}

}  // namespace

Exception BleL2capOutputStream::Write(absl::string_view data) {
  if (data.size() > std::numeric_limits<uint32_t>::max()) {
    return {Exception::kIo};
  }

  std::string frame = EncodeFrameLength(static_cast<uint32_t>(data.size()));
  frame.append(data.data(), data.size());
  return stream_.Write(frame);
}

Exception BleL2capOutputStream::Flush() {
  return stream_.Flush();
}

ExceptionOr<ByteArray> BleL2capInputStream::Read(std::int64_t size) {
  if (size <= 0) {
    return ExceptionOr<ByteArray>(ByteArray(std::string()));
  }

  while (pending_.empty()) {
    ExceptionOr<ByteArray> packet = stream_.Read(kMaxFrameSize);
    if (!packet.ok()) {
      return {Exception::kIo};
    }

    std::string packet_data = packet.result().string_data();

    if (packet_data.empty()) {
      return {Exception::kIo};
    }

    wire_buffer_.append(packet_data);

    while (wire_buffer_.size() >= kFrameHeaderSize) {
      uint32_t frame_length = DecodeFrameLength(absl::string_view(
          wire_buffer_.data(), kFrameHeaderSize));
      if (frame_length > kMaxFrameSize) {
        LOG(ERROR) << __func__ << ": invalid L2CAP frame length "
                   << frame_length;
        return {Exception::kIo};
      }

      size_t full_frame_size = kFrameHeaderSize + frame_length;
      if (wire_buffer_.size() < full_frame_size) {
        break;
      }

      pending_.append(wire_buffer_.data() + kFrameHeaderSize, frame_length);
      wire_buffer_.erase(0, full_frame_size);

      if (!pending_.empty()) {
        break;
      }

      continue;
    }
  }

  size_t bytes_to_return = std::min(static_cast<size_t>(size), pending_.size());
  std::string out = pending_.substr(0, bytes_to_return);
  pending_.erase(0, bytes_to_return);
  return ExceptionOr<ByteArray>(ByteArray(std::move(out)));
}
}  // namespace linux
}  // namespace nearby
