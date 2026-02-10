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

#ifndef PLATFORM_IMPL_LINUX_BLE_L2CAP_SOCKET_H_
#define PLATFORM_IMPL_LINUX_BLE_L2CAP_SOCKET_H_

#include <atomic>
#include <memory>
#include <optional>
#include <string>

#include "absl/functional/any_invocable.h"
#include "absl/synchronization/mutex.h"
#include "absl/time/time.h"
#include "internal/platform/byte_array.h"
#include "internal/platform/exception.h"
#include "internal/platform/implementation/ble.h"
#include "internal/platform/input_stream.h"
#include "internal/platform/output_stream.h"

namespace nearby {
namespace linux {

class BleL2capSocket;

class BleL2capInputStream final : public InputStream {
 public:
  explicit BleL2capInputStream(BleL2capSocket* owner);
  ~BleL2capInputStream() override;

  ExceptionOr<ByteArray> Read(std::int64_t size) override;
  Exception Close() override;

 private:
  BleL2capSocket* owner_ = nullptr;
};

class BleL2capOutputStream final : public OutputStream {
 public:
  explicit BleL2capOutputStream(BleL2capSocket* owner);
  ~BleL2capOutputStream() override;

  Exception Write(absl::string_view data) override;
  Exception Flush() override { return {Exception::kSuccess}; }
  Exception Close() override;

 private:
  BleL2capSocket* owner_ = nullptr;
};

class BleL2capSocket final : public api::ble::BleL2capSocket {
 public:
  enum class ProtocolMode {
    kRefactored,
    kLegacy,
  };

  BleL2capSocket(int fd, api::ble::BlePeripheral::UniqueId peripheral_id,
                 ProtocolMode protocol_mode = ProtocolMode::kRefactored,
                 std::string service_id = "", bool incoming_connection = false);
  ~BleL2capSocket() override;

  InputStream& GetInputStream() override { return *input_stream_; }
  OutputStream& GetOutputStream() override { return *output_stream_; }
  Exception Close() override ABSL_LOCKS_EXCLUDED(mutex_);
  void SetCloseNotifier(absl::AnyInvocable<void()> notifier) override
      ABSL_LOCKS_EXCLUDED(mutex_);
  api::ble::BlePeripheral::UniqueId GetRemotePeripheralId() override {
    return peripheral_id_;
  }

  bool IsClosed() const ABSL_LOCKS_EXCLUDED(mutex_);
  bool PerformLegacyOutgoingHandshake(absl::Duration timeout);

 private:
  friend class BleL2capInputStream;
  friend class BleL2capOutputStream;

  enum class LegacyControlCommand : uint8_t {
    kRequestAdvertisement = 1,
    kRequestAdvertisementFinish = 2,
    kRequestDataConnection = 3,
    kResponseAdvertisement = 21,
    kResponseServiceIdNotFound = 22,
    kResponseDataConnectionReady = 23,
    kResponseDataConnectionFailure = 24,
  };

  struct ParsedLegacyControlPacket {
    LegacyControlCommand command;
    ByteArray data;
  };

  ExceptionOr<ByteArray> ReadFromSocket(std::int64_t size)
      ABSL_LOCKS_EXCLUDED(io_mutex_);
  Exception WriteToSocket(absl::string_view data) ABSL_LOCKS_EXCLUDED(io_mutex_);
  Exception CloseIo() ABSL_LOCKS_EXCLUDED(io_mutex_);

  bool ReadNextFrame(std::string& payload, std::optional<absl::Duration> timeout)
      ABSL_LOCKS_EXCLUDED(io_mutex_);
  bool SendFrame(absl::string_view payload) ABSL_LOCKS_EXCLUDED(io_mutex_);
  bool SendLegacyControlPacket(LegacyControlCommand command,
                               const ByteArray& data) ABSL_LOCKS_EXCLUDED(io_mutex_);
  bool SendLegacyIntroductionPacket() ABSL_LOCKS_EXCLUDED(io_mutex_);
  bool SendLegacyPacketAckPacket(int received_size) ABSL_LOCKS_EXCLUDED(io_mutex_);
  std::optional<ParsedLegacyControlPacket> ParseLegacyControlPacket(
      absl::string_view payload) const;
  std::optional<ByteArray> ParseLegacyIntroductionPacket(
      absl::string_view payload) const;
  bool HandleLegacyIncomingPayload(absl::string_view payload, bool& produced_payload)
      ABSL_LOCKS_EXCLUDED(io_mutex_);

  static bool IsSupportedLegacyControlCommand(uint8_t command);
  static const char* LegacyControlCommandToString(LegacyControlCommand command);
  static ByteArray BuildLegacyControlPacket(LegacyControlCommand command,
                                            const ByteArray& data);

  bool PollReady(short events, std::optional<absl::Duration> timeout) const;
  void DoClose() ABSL_EXCLUSIVE_LOCKS_REQUIRED(mutex_);

  mutable absl::Mutex mutex_;
  mutable absl::Mutex io_mutex_;
  bool closed_ ABSL_GUARDED_BY(mutex_) = false;
  std::unique_ptr<BleL2capInputStream> input_stream_;
  std::unique_ptr<BleL2capOutputStream> output_stream_;
  api::ble::BlePeripheral::UniqueId peripheral_id_;
  absl::AnyInvocable<void()> close_notifier_ ABSL_GUARDED_BY(mutex_);
  std::atomic<int> fd_{-1};

  const ProtocolMode protocol_mode_;
  const bool incoming_connection_;
  ByteArray service_id_hash_;

  bool intro_packet_validated_ = false;
  bool request_data_connection_handled_ = false;
  std::string wire_buffer_;
  std::string read_buffer_;
};

}  // namespace linux
}  // namespace nearby

#endif  // PLATFORM_IMPL_LINUX_BLE_L2CAP_SOCKET_H_
