// Copyright 2023 Google LLC
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

#ifndef PLATFORM_IMPL_LINUX_BLUETOOTH_SOCKET_H_
#define PLATFORM_IMPL_LINUX_BLUETOOTH_SOCKET_H_

#include <atomic>
#include <memory>
#include <optional>

#include <sdbus-c++/Types.h>
#include <sys/poll.h>
#include <systemd/sd-bus.h>

#include "absl/synchronization/mutex.h"
#include "internal/platform/exception.h"
#include "internal/platform/implementation/bluetooth_classic.h"
#include "internal/platform/implementation/linux/bluetooth_classic_device.h"

#include "internal/platform/implementation/linux/stream.h"

namespace nearby {
namespace linux {


class BluetoothSocket final : public api::BluetoothSocket {
 public:
  BluetoothSocket(std::shared_ptr<BluetoothDevice> device,
                  sdbus::UnixFd fd)
      :fd_(fd), device_(std::move(device)), output_stream_(fd_), input_stream_(fd_) {}

  InputStream &GetInputStream() override { return input_stream_; }
  OutputStream &GetOutputStream() override { return output_stream_; }
  Exception Close() override {
    input_stream_.Close();
    output_stream_.Close();

    return Exception{Exception::kSuccess};
  }
  api::BluetoothDevice *GetRemoteDevice() override { return device_.get(); };

 private:
  sdbus::UnixFd fd_;
  std::shared_ptr<BluetoothDevice> device_;
  OutputStream output_stream_;
  InputStream input_stream_;
};
}  // namespace linux
}  // namespace nearby
#endif
