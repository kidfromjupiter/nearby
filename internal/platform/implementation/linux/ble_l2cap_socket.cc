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

#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <optional>
#include <string>
#include <utility>

#include "absl/strings/string_view.h"
#include "absl/time/time.h"
#include "internal/platform/implementation/crypto.h"
#include "internal/platform/logging.h"
#include "proto/mediums/ble_frames.pb.h"

namespace nearby {
namespace linux {
namespace {

constexpr int kHeaderLength = 4;
constexpr int kServiceIdHashLength = 3;
constexpr int kMaxFrameLength = 1024 * 1024;
constexpr uint8_t kControlPacketPrefix[kServiceIdHashLength] = {0x00, 0x00, 0x00};

using ::location::nearby::mediums::SocketControlFrame;
using ::location::nearby::mediums::SocketVersion;

struct ParsedSocketControlFrame {
  SocketControlFrame::ControlFrameType type;
  ByteArray service_id_hash;
  int received_size = 0;
};

std::optional<ParsedSocketControlFrame> ParseSocketControlFramePayload(
    absl::string_view payload) {
  if (payload.size() <= kServiceIdHashLength) return std::nullopt;
  if (!std::equal(std::begin(kControlPacketPrefix), std::end(kControlPacketPrefix),
                  payload.begin())) {
    return std::nullopt;
  }

  SocketControlFrame frame;
  if (!frame.ParseFromArray(payload.data() + kServiceIdHashLength,
                            payload.size() - kServiceIdHashLength)) {
    return std::nullopt;
  }

  ParsedSocketControlFrame parsed{
      .type = frame.type(),
      .service_id_hash = ByteArray(),
      .received_size = 0,
  };

  switch (frame.type()) {
    case SocketControlFrame::INTRODUCTION:
      if (!frame.has_introduction() || !frame.introduction().has_service_id_hash() ||
          frame.introduction().socket_version() != SocketVersion::V2) {
        return std::nullopt;
      }
      parsed.service_id_hash =
          ByteArray(frame.introduction().service_id_hash().data(),
                    frame.introduction().service_id_hash().size());
      return parsed;
    case SocketControlFrame::DISCONNECTION:
      if (!frame.has_disconnection() || !frame.disconnection().has_service_id_hash()) {
        return std::nullopt;
      }
      parsed.service_id_hash =
          ByteArray(frame.disconnection().service_id_hash().data(),
                    frame.disconnection().service_id_hash().size());
      return parsed;
    case SocketControlFrame::PACKET_ACKNOWLEDGEMENT:
      if (!frame.has_packet_acknowledgement() ||
          !frame.packet_acknowledgement().has_service_id_hash()) {
        return std::nullopt;
      }
      parsed.service_id_hash =
          ByteArray(frame.packet_acknowledgement().service_id_hash().data(),
                    frame.packet_acknowledgement().service_id_hash().size());
      parsed.received_size = frame.packet_acknowledgement().has_received_size()
                                 ? frame.packet_acknowledgement().received_size()
                                 : 0;
      return parsed;
    case SocketControlFrame::UNKNOWN_CONTROL_FRAME_TYPE:
      return std::nullopt;
  }
  return std::nullopt;
}

std::optional<uint32_t> ReadBigEndianUint32(absl::string_view bytes) {
  if (bytes.size() != kHeaderLength) return std::nullopt;
  const auto* ptr = reinterpret_cast<const uint8_t*>(bytes.data());
  return (static_cast<uint32_t>(ptr[0]) << 24) |
         (static_cast<uint32_t>(ptr[1]) << 16) |
         (static_cast<uint32_t>(ptr[2]) << 8) |
         static_cast<uint32_t>(ptr[3]);
}

std::string WriteBigEndianUint32(uint32_t value) {
  std::string out(kHeaderLength, '\0');
  out[0] = static_cast<char>((value >> 24) & 0xFF);
  out[1] = static_cast<char>((value >> 16) & 0xFF);
  out[2] = static_cast<char>((value >> 8) & 0xFF);
  out[3] = static_cast<char>(value & 0xFF);
  return out;
}

}  // namespace

BleL2capInputStream::BleL2capInputStream(BleL2capSocket* owner) : owner_(owner) {}

BleL2capInputStream::~BleL2capInputStream() { Close(); }

ExceptionOr<ByteArray> BleL2capInputStream::Read(std::int64_t size) {
  if (owner_ == nullptr) {
    return ExceptionOr<ByteArray>(Exception::kIo);
  }
  return owner_->ReadFromSocket(size);
}

Exception BleL2capInputStream::Close() {
  if (owner_ == nullptr) return {Exception::kSuccess};
  return owner_->CloseIo();
}

BleL2capOutputStream::BleL2capOutputStream(BleL2capSocket* owner) : owner_(owner) {}

BleL2capOutputStream::~BleL2capOutputStream() { Close(); }

Exception BleL2capOutputStream::Write(absl::string_view data) {
  if (owner_ == nullptr) {
    return {Exception::kIo};
  }
  return owner_->WriteToSocket(data);
}

Exception BleL2capOutputStream::Close() {
  if (owner_ == nullptr) return {Exception::kSuccess};
  return owner_->CloseIo();
}

BleL2capSocket::BleL2capSocket(int fd,
                               api::ble::BlePeripheral::UniqueId peripheral_id,
                               ProtocolMode protocol_mode,
                               std::string service_id,
                               bool incoming_connection)
    : input_stream_(std::make_unique<BleL2capInputStream>(this)),
      output_stream_(std::make_unique<BleL2capOutputStream>(this)),
      peripheral_id_(peripheral_id),
      fd_(fd),
      protocol_mode_(protocol_mode),
      incoming_connection_(incoming_connection),
      intro_packet_validated_(!incoming_connection) {
  if (protocol_mode_ == ProtocolMode::kLegacy) {
    ByteArray hash = Crypto::Sha256(service_id);
    if (hash.size() < kServiceIdHashLength) {
      LOG(ERROR) << "Failed to derive service hash for legacy L2CAP mode.";
    } else {
      service_id_hash_ = ByteArray(hash.data(), kServiceIdHashLength);
    }
  }
}

BleL2capSocket::~BleL2capSocket() { Close(); }

bool BleL2capSocket::IsSupportedLegacyControlCommand(uint8_t command) {
  switch (command) {
    case static_cast<uint8_t>(LegacyControlCommand::kRequestAdvertisement):
    case static_cast<uint8_t>(LegacyControlCommand::kRequestAdvertisementFinish):
    case static_cast<uint8_t>(LegacyControlCommand::kRequestDataConnection):
    case static_cast<uint8_t>(LegacyControlCommand::kResponseAdvertisement):
    case static_cast<uint8_t>(LegacyControlCommand::kResponseServiceIdNotFound):
    case static_cast<uint8_t>(LegacyControlCommand::kResponseDataConnectionReady):
    case static_cast<uint8_t>(LegacyControlCommand::kResponseDataConnectionFailure):
      return true;
    default:
      return false;
  }
}

const char* BleL2capSocket::LegacyControlCommandToString(
    LegacyControlCommand command) {
  switch (command) {
    case LegacyControlCommand::kRequestAdvertisement:
      return "RequestAdvertisement(1)";
    case LegacyControlCommand::kRequestAdvertisementFinish:
      return "RequestAdvertisementFinish(2)";
    case LegacyControlCommand::kRequestDataConnection:
      return "RequestDataConnection(3)";
    case LegacyControlCommand::kResponseAdvertisement:
      return "ResponseAdvertisement(21)";
    case LegacyControlCommand::kResponseServiceIdNotFound:
      return "ResponseServiceIdNotFound(22)";
    case LegacyControlCommand::kResponseDataConnectionReady:
      return "ResponseDataConnectionReady(23)";
    case LegacyControlCommand::kResponseDataConnectionFailure:
      return "ResponseDataConnectionFailure(24)";
  }
  return "UnknownCommand";
}

ByteArray BleL2capSocket::BuildLegacyControlPacket(LegacyControlCommand command,
                                                   const ByteArray& data) {
  std::string packet;
  packet.push_back(static_cast<char>(command));
  if (!data.Empty()) {
    if (data.size() > 0xFFFF) return ByteArray();
    packet.push_back(static_cast<char>((data.size() >> 8) & 0xFF));
    packet.push_back(static_cast<char>(data.size() & 0xFF));
    packet.append(data.data(), data.size());
  }
  return ByteArray(std::move(packet));
}

std::optional<BleL2capSocket::ParsedLegacyControlPacket>
BleL2capSocket::ParseLegacyControlPacket(absl::string_view payload) const {
  if (payload.empty()) {
    return std::nullopt;
  }

  const uint8_t command = static_cast<uint8_t>(payload[0]);
  if (!IsSupportedLegacyControlCommand(command)) {
    return std::nullopt;
  }

  ParsedLegacyControlPacket packet{
      .command = static_cast<LegacyControlCommand>(command),
      .data = ByteArray(),
  };

  if (payload.size() == 1) {
    return packet;
  }

  if (payload.size() < 3) {
    return std::nullopt;
  }

  const auto* bytes = reinterpret_cast<const uint8_t*>(payload.data());
  const int length = (static_cast<int>(bytes[1]) << 8) | static_cast<int>(bytes[2]);
  if (length != static_cast<int>(payload.size()) - 3) {
    return std::nullopt;
  }

  if (length > 0) {
    packet.data = ByteArray(payload.data() + 3, static_cast<size_t>(length));
  }
  return packet;
}

std::optional<ByteArray> BleL2capSocket::ParseLegacyIntroductionPacket(
    absl::string_view payload) const {
  auto parsed = ParseSocketControlFramePayload(payload);
  if (!parsed.has_value() || parsed->type != SocketControlFrame::INTRODUCTION) {
    return std::nullopt;
  }
  return parsed->service_id_hash;
}

bool BleL2capSocket::PollReady(short events, std::optional<absl::Duration> timeout) const {
  int fd = fd_.load();
  if (fd < 0) return false;

  int timeout_ms = -1;
  if (timeout.has_value()) {
    timeout_ms = std::max<int64_t>(0, absl::ToInt64Milliseconds(*timeout));
  }

  struct pollfd pfd;
  pfd.fd = fd;
  pfd.events = events;
  pfd.revents = 0;

  while (true) {
    int ret = poll(&pfd, 1, timeout_ms);
    if (ret < 0) {
      if (errno == EINTR) continue;
      return false;
    }
    if (ret == 0) return false;

    if ((pfd.revents & events) != 0) return true;
    if ((pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) != 0) {
      return false;
    }
  }
}

bool BleL2capSocket::SendFrame(absl::string_view payload) {
  if (payload.size() > kMaxFrameLength) {
    return false;
  }
  std::string framed = WriteBigEndianUint32(static_cast<uint32_t>(payload.size()));
  framed.append(payload.data(), payload.size());

  absl::MutexLock lock(&io_mutex_);
  int fd = fd_.load();
  if (fd < 0) return false;

  size_t offset = 0;
  while (offset < framed.size()) {
    if (!PollReady(POLLOUT, std::nullopt)) {
      return false;
    }
    fd = fd_.load();
    if (fd < 0) return false;

    ssize_t sent = send(
        fd, framed.data() + offset, framed.size() - offset,
#ifdef MSG_NOSIGNAL
        MSG_NOSIGNAL
#else
        0
#endif
    );
    if (sent < 0) {
      if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) continue;
      return false;
    }
    if (sent == 0) return false;
    offset += static_cast<size_t>(sent);
  }
  return true;
}

bool BleL2capSocket::ReadNextFrame(std::string& payload,
                                   std::optional<absl::Duration> timeout) {
  const std::optional<absl::Time> deadline =
      timeout.has_value() ? std::make_optional(absl::Now() + *timeout) : std::nullopt;

  while (true) {
    if (wire_buffer_.size() >= kHeaderLength) {
      auto frame_length = ReadBigEndianUint32(
          absl::string_view(wire_buffer_.data(), kHeaderLength));
      if (!frame_length.has_value() || *frame_length > kMaxFrameLength) {
        return false;
      }
      const size_t total = kHeaderLength + *frame_length;
      if (wire_buffer_.size() >= total) {
        payload = wire_buffer_.substr(kHeaderLength, *frame_length);
        wire_buffer_.erase(0, total);
        return true;
      }
    }

    std::optional<absl::Duration> remaining = std::nullopt;
    if (deadline.has_value()) {
      remaining = *deadline - absl::Now();
      if (*remaining <= absl::ZeroDuration()) {
        return false;
      }
    }

    if (!PollReady(POLLIN, remaining)) return false;

    int fd = fd_.load();
    if (fd < 0) return false;

    char buffer[1024];
    ssize_t read_count = recv(fd, buffer, sizeof(buffer), 0);
    if (read_count < 0) {
      if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) continue;
      return false;
    }
    if (read_count == 0) return false;
    wire_buffer_.append(buffer, static_cast<size_t>(read_count));
  }
}

bool BleL2capSocket::SendLegacyControlPacket(LegacyControlCommand command,
                                             const ByteArray& data) {
  ByteArray packet = BuildLegacyControlPacket(command, data);
  if (packet.Empty()) return false;
  return SendFrame(packet.AsStringView());
}

bool BleL2capSocket::SendLegacyIntroductionPacket() {
  if (service_id_hash_.size() != kServiceIdHashLength) {
    return false;
  }

  SocketControlFrame frame;
  frame.set_type(SocketControlFrame::INTRODUCTION);
  auto* intro = frame.mutable_introduction();
  intro->set_socket_version(SocketVersion::V2);
  intro->set_service_id_hash(service_id_hash_.AsStringView());

  ByteArray frame_bytes(frame.ByteSizeLong());
  if (!frame.SerializeToArray(frame_bytes.data(), frame_bytes.size())) return false;

  std::string packet(reinterpret_cast<const char*>(kControlPacketPrefix),
                     kServiceIdHashLength);
  packet.append(frame_bytes.data(), frame_bytes.size());
  return SendFrame(packet);
}

bool BleL2capSocket::SendLegacyPacketAckPacket(int received_size) {
  if (service_id_hash_.size() != kServiceIdHashLength) {
    return false;
  }

  SocketControlFrame frame;
  frame.set_type(SocketControlFrame::PACKET_ACKNOWLEDGEMENT);
  auto* ack = frame.mutable_packet_acknowledgement();
  ack->set_service_id_hash(service_id_hash_.AsStringView());
  ack->set_received_size(received_size);

  ByteArray frame_bytes(frame.ByteSizeLong());
  if (!frame.SerializeToArray(frame_bytes.data(), frame_bytes.size())) return false;

  std::string packet(reinterpret_cast<const char*>(kControlPacketPrefix),
                     kServiceIdHashLength);
  packet.append(frame_bytes.data(), frame_bytes.size());
  return SendFrame(packet);
}

bool BleL2capSocket::HandleLegacyIncomingPayload(absl::string_view payload,
                                                 bool& produced_payload) {
  produced_payload = false;

  if (auto control_packet = ParseLegacyControlPacket(payload);
      control_packet.has_value()) {
    switch (control_packet->command) {
      case LegacyControlCommand::kRequestDataConnection:
        if (!incoming_connection_ || request_data_connection_handled_) return false;
        request_data_connection_handled_ = true;
        return SendLegacyControlPacket(
            LegacyControlCommand::kResponseDataConnectionReady, ByteArray());
      case LegacyControlCommand::kRequestAdvertisement:
        // Mirror Apple behavior: return an empty advertisement response.
        return SendLegacyControlPacket(
            LegacyControlCommand::kResponseAdvertisement, ByteArray());
      default:
        return false;
    }
  }

  if (auto control_frame = ParseSocketControlFramePayload(payload);
      control_frame.has_value()) {
    if (control_frame->service_id_hash != service_id_hash_) return false;

    switch (control_frame->type) {
      case SocketControlFrame::INTRODUCTION:
        intro_packet_validated_ = true;
        return true;
      case SocketControlFrame::PACKET_ACKNOWLEDGEMENT:
        // ACK frames are control-plane signals and should not be surfaced as
        // payload bytes to upper layers.
        return true;
      case SocketControlFrame::DISCONNECTION:
        return false;
      case SocketControlFrame::UNKNOWN_CONTROL_FRAME_TYPE:
        return false;
    }
  }

  if (service_id_hash_.size() != kServiceIdHashLength ||
      payload.size() < service_id_hash_.size()) {
    return false;
  }

  if (!std::equal(service_id_hash_.data(),
                  service_id_hash_.data() + service_id_hash_.size(),
                  payload.data())) {
    return false;
  }

  if (incoming_connection_ && !intro_packet_validated_) return false;

  read_buffer_.append(payload.data() + service_id_hash_.size(),
                      payload.size() - service_id_hash_.size());
  produced_payload = true;
  return SendLegacyPacketAckPacket(static_cast<int>(payload.size()));
}

ExceptionOr<ByteArray> BleL2capSocket::ReadFromSocket(std::int64_t size) {
  if (size <= 0) return ExceptionOr<ByteArray>(ByteArray());

  while (read_buffer_.empty()) {
    std::string payload;
    if (!ReadNextFrame(payload, std::nullopt))
      return ExceptionOr<ByteArray>(Exception::kIo);

    if (protocol_mode_ == ProtocolMode::kRefactored) {
      read_buffer_.append(payload);
      continue;
    }

    bool produced_payload = false;
    if (!HandleLegacyIncomingPayload(payload, produced_payload)) {
      CloseIo();
      return ExceptionOr<ByteArray>(Exception::kIo);
    }

    if (!produced_payload) {
      continue;
    }
  }

  const size_t read_size =
      std::min<size_t>(static_cast<size_t>(size), read_buffer_.size());
  ByteArray result(read_buffer_.substr(0, read_size));
  read_buffer_.erase(0, read_size);
  return ExceptionOr<ByteArray>(result);
}

Exception BleL2capSocket::WriteToSocket(absl::string_view data) {
  if (protocol_mode_ == ProtocolMode::kLegacy) {
    if (service_id_hash_.size() != kServiceIdHashLength) return {Exception::kIo};

    std::string payload;
    payload.reserve(service_id_hash_.size() + data.size());
    payload.append(service_id_hash_.AsStringView());
    payload.append(data.data(), data.size());

    return SendFrame(payload) ? Exception{Exception::kSuccess}
                              : Exception{Exception::kIo};
  }

  return SendFrame(data) ? Exception{Exception::kSuccess}
                         : Exception{Exception::kIo};
}

Exception BleL2capSocket::CloseIo() {
  int fd = fd_.exchange(-1);
  if (fd < 0) return {Exception::kSuccess};

  shutdown(fd, SHUT_RDWR);
  return {Exception::kSuccess};
}

bool BleL2capSocket::PerformLegacyOutgoingHandshake(absl::Duration timeout) {
  if (protocol_mode_ != ProtocolMode::kLegacy) return true;

  if (service_id_hash_.size() != kServiceIdHashLength) {
    LOG(ERROR) << "Legacy L2CAP handshake requires valid service id hash.";
    return false;
  }

  if (!SendLegacyControlPacket(LegacyControlCommand::kRequestDataConnection,
                               ByteArray())) {
    return false;
  }

  const absl::Time deadline = absl::Now() + timeout;
  while (absl::Now() < deadline) {
    std::string payload;
    if (!ReadNextFrame(payload, deadline - absl::Now())) return false;

    auto control_packet = ParseLegacyControlPacket(payload);
    if (!control_packet.has_value()) return false;

    if (control_packet->command ==
        LegacyControlCommand::kResponseDataConnectionReady) {
      return SendLegacyIntroductionPacket();
    }

    // Strict fail-fast for unexpected commands in handshake phase.
    return false;
  }

  return false;
}

Exception BleL2capSocket::Close() {
  absl::MutexLock lock(&mutex_);
  if (closed_) {
    return {Exception::kSuccess};
  }
  DoClose();
  return {Exception::kSuccess};
}

void BleL2capSocket::DoClose() {
  closed_ = true;

  if (input_stream_) {
    input_stream_->Close();
  }
  if (output_stream_) {
    output_stream_->Close();
  }

  if (close_notifier_) {
    auto notifier = std::move(close_notifier_);
    mutex_.Unlock();
    notifier();
    mutex_.Lock();
  }
}

void BleL2capSocket::SetCloseNotifier(absl::AnyInvocable<void()> notifier) {
  absl::MutexLock lock(&mutex_);
  close_notifier_ = std::move(notifier);
}

bool BleL2capSocket::IsClosed() const {
  absl::MutexLock lock(&mutex_);
  return closed_;
}

}  // namespace linux
}  // namespace nearby
