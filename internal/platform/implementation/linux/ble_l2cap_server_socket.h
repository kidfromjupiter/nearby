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

#ifndef PLATFORM_IMPL_LINUX_BLE_L2CAP_SERVER_SOCKET_H_
#define PLATFORM_IMPL_LINUX_BLE_L2CAP_SERVER_SOCKET_H_

#include <memory>
#include <string>

#include "absl/functional/any_invocable.h"
#include "absl/synchronization/mutex.h"
#include "internal/platform/exception.h"
#include "internal/platform/implementation/ble.h"
#include "internal/platform/implementation/linux/ble_l2cap_socket.h"

#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
namespace nearby {
namespace linux {

class BleL2capServerSocket final : public api::ble::BleL2capServerSocket {
 public:
  BleL2capServerSocket();
  explicit BleL2capServerSocket(
      int psm,
      BleL2capSocket::ProtocolMode protocol_mode = BleL2capSocket::ProtocolMode::kRefactored,
      std::string service_id = "");
  ~BleL2capServerSocket() override;

  int GetPSM() const override { return psm_; }
  void SetPSM(int psm);

  std::unique_ptr<api::ble::BleL2capSocket> Accept() override
      ABSL_LOCKS_EXCLUDED(mutex_);
  Exception Close() override ABSL_LOCKS_EXCLUDED(mutex_);

  void SetCloseNotifier(absl::AnyInvocable<void()> notifier)
      ABSL_LOCKS_EXCLUDED(mutex_);

 private:
  bool InitializeServerSocketLocked() ABSL_EXCLUSIVE_LOCKS_REQUIRED(mutex_);
  void AcceptPoll(int server_fd, int stop_fd, int& client_fd,
                  sockaddr_l2& client_addr, socklen_t& client_len);

  absl::Mutex mutex_;
  bool closed_ ABSL_GUARDED_BY(mutex_) = false;
  absl::AnyInvocable<void()> close_notifier_ ABSL_GUARDED_BY(mutex_);
  int psm_ = 0;
  BleL2capSocket::ProtocolMode protocol_mode_ = BleL2capSocket::ProtocolMode::kRefactored;
  std::string service_id_ ABSL_GUARDED_BY(mutex_);
  int server_fd_ ABSL_GUARDED_BY(mutex_) = -1;
  int stop_pipe_[2] ABSL_GUARDED_BY(mutex_) = {-1, -1};  // read [0], write [1]
};

}  // namespace linux
}  // namespace nearby

#endif  // PLATFORM_IMPL_LINUX_BLE_L2CAP_SERVER_SOCKET_H_
