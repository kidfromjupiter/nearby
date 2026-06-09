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

#include "dbus.h"


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

// TODO: use linux stream instead of bespoke l2cap input/output stream

class BleL2capSocket;

class BleL2capInputStream final : public InputStream {
 public:
  explicit BleL2capInputStream(std::shared_ptr<sdbus::UnixFd> fd_raw_): fd_raw_(std::move(fd_raw_)) {};
  ~BleL2capInputStream() override;

  ExceptionOr<ByteArray> Read(std::int64_t size) override;
  Exception Close() override;

private:
  std::shared_ptr<sdbus::UnixFd> fd_raw_;
};

class BleL2capOutputStream final : public OutputStream {
public:
  explicit BleL2capOutputStream(std::shared_ptr<sdbus::UnixFd> fd_raw_): fd_raw_(std::move(fd_raw_)) {};
  ~BleL2capOutputStream() override;

  Exception Write(absl::string_view data) override;
  Exception Flush() override { return {Exception::kSuccess}; }
  Exception Close() override;

private:
  std::shared_ptr<sdbus::UnixFd> fd_raw_;

};

class BleL2capSocket final : public api::ble::BleL2capSocket {
 public:

  BleL2capSocket(int fd, api::ble::BlePeripheral::UniqueId peripheral_id,
                 std::string service_id = "");
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

 private:
  friend class BleL2capInputStream;
  friend class BleL2capOutputStream;

  mutable absl::Mutex mutex_;
  mutable absl::Mutex io_mutex_;
  bool closed_ ABSL_GUARDED_BY(mutex_) = false;
  std::shared_ptr<sdbus::UnixFd > fd_ ;
  std::unique_ptr<BleL2capInputStream> input_stream_;
  std::unique_ptr<BleL2capOutputStream> output_stream_;
  api::ble::BlePeripheral::UniqueId peripheral_id_;
};

}  // namespace linux
}  // namespace nearby

#endif  // PLATFORM_IMPL_LINUX_BLE_L2CAP_SOCKET_H_
