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

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unistd.h>

#include "absl/functional/any_invocable.h"
#include "absl/synchronization/mutex.h"
#include "absl/time/time.h"
#include "internal/platform/byte_array.h"
#include "internal/platform/exception.h"
#include "internal/platform/implementation/ble.h"
#include "internal/platform/implementation/linux/stream.h"

namespace nearby {
namespace linux {

class BleL2capSocket;

class BleL2capInputStream : public nearby::InputStream {
 public:
  explicit BleL2capInputStream(int fd) : stream_(fd) {}

  ExceptionOr<ByteArray> Read(std::int64_t size) override;
  Exception Close() override {
    if (closed_) {
      return {Exception::kSuccess};
    }
    closed_ = true;
    return stream_.Close();
  }

 private:
  linux::InputStream stream_;
  std::string wire_buffer_;
  std::string pending_;
  bool closed_ = false;
};

class BleL2capOutputStream : public nearby::OutputStream {
 public:
  explicit BleL2capOutputStream(int fd) : stream_(fd) {}

  Exception Write(absl::string_view data) override;
  Exception Flush() override;
  Exception Close() override {
    if (closed_) {
      return {Exception::kSuccess};
    }
    closed_ = true;
    return stream_.Close();
  }

 private:
  linux::OutputStream stream_;
  bool closed_ = false;
};

class BleL2capSocket final : public api::ble::BleL2capSocket {
 public:
  BleL2capSocket(int fd, api::ble::BlePeripheral::UniqueId peripheral_id,
                 std::string service_id = "")
      : fd_(fd),
        output_stream_(fd_),
        input_stream_(fd_),
        peripheral_id_(peripheral_id) {};

  nearby::InputStream& GetInputStream() override { return input_stream_; }
  nearby::OutputStream& GetOutputStream() override { return output_stream_; }
  Exception Close() override {
    input_stream_.Close();
    output_stream_.Close();
    if (fd_ >= 0) {
      close(fd_);
      fd_ = -1;
    }

    return Exception{Exception::kSuccess};
  };
  api::ble::BlePeripheral::UniqueId GetRemotePeripheralId() override {
    return peripheral_id_;
  }

 private:
  int fd_;
  BleL2capOutputStream output_stream_;
  BleL2capInputStream input_stream_;
  api::ble::BlePeripheral::UniqueId peripheral_id_;
};

}  // namespace linux
}  // namespace nearby

#endif  // PLATFORM_IMPL_LINUX_BLE_L2CAP_SOCKET_H_
