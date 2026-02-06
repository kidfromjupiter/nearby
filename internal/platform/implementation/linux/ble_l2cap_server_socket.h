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
#include <sdbus-c++/Types.h>

#include "absl/functional/any_invocable.h"
#include "absl/synchronization/mutex.h"
#include "internal/platform/exception.h"
#include "internal/platform/implementation/ble.h"
#include "internal/platform/implementation/ble.h"
#include "internal/platform/implementation/linux/ble_l2cap_socket.h"
#include "absl/container/flat_hash_map.h"

#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
namespace nearby {
namespace linux {

class BleL2capServerSocket final : public api::ble::BleL2capServerSocket {
 public:
  BleL2capServerSocket();
  explicit BleL2capServerSocket(int psm);
  ~BleL2capServerSocket() override;

  int GetPSM() const override { return psm_; }
  void SetPSM(int psm);
  void AcceptPoll(int &client_fd, sockaddr_l2 &client_addr,
                  socklen_t &client_len);

  std::unique_ptr<api::ble::BleL2capSocket> Accept() override;
  Exception Close() override ;

  void SetCloseNotifier(absl::AnyInvocable<void()> notifier);

 private:
  mutable absl::Mutex mutex_;
  absl::CondVar cond_;
  int psm_ = 0;
  int server_fd_ = -1;
  int stop_pipe_[2] = {-1, -1};   // read end [0], write end [1]

  // <server_fd, <client_fd, peripheral_id>>
  absl::flat_hash_map<int, std::pair<int,api::ble::BlePeripheral::UniqueId>> accepted_fds_;
};

}  // namespace linux
}  // namespace nearby

#endif  // PLATFORM_IMPL_LINUX_BLE_L2CAP_SERVER_SOCKET_H_
