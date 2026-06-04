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

#include "internal/platform/implementation/linux/ble_l2cap_server_socket.h"

#include <fcntl.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstdint>
#include <cstring>
#include <utility>

#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>

#include "internal/platform/logging.h"

#include "internal/platform/prng.h"

namespace nearby {
namespace linux {

BleL2capServerSocket::BleL2capServerSocket() = default;

BleL2capServerSocket::BleL2capServerSocket(int psm, std::string service_id)
    : psm_(psm), service_id_(std::move(service_id)) {}

BleL2capServerSocket::~BleL2capServerSocket() {
  Close();
}

void BleL2capServerSocket::SetPSM(int psm) {
  absl::MutexLock lock(&mutex_);
  psm_ = psm;
}

std::unique_ptr<api::ble::BleL2capSocket> BleL2capServerSocket::Accept() {
  {
    server_fd_ = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
    if (server_fd_ < 0) {
      LOG(ERROR) << "Failed to create L2CAP server socket: "
                 << std::strerror(errno);
      return nullptr;
    }

    // generating psm value for l2cap socket
    Prng prng;
    psm_ = 0x80 + (prng.NextUint32() % 0x80);

    sockaddr_l2 addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.l2_family = AF_BLUETOOTH;
    addr.l2_psm = htobs(psm_);
    addr.l2_bdaddr_type = BDADDR_LE_PUBLIC;
    std::memset(&addr.l2_bdaddr, 0, sizeof(addr.l2_bdaddr));

    if (bind(server_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) <
        0) {
      LOG(ERROR) << "Failed to bind L2CAP server socket: "
                 << std::strerror(errno) << " (errno: " << errno << ")";
      close(server_fd_);
      server_fd_ = -1;
      return nullptr;
    }

    constexpr uint16_t kReceiveMtu = 672;
    if (setsockopt(server_fd_, SOL_BLUETOOTH, BT_RCVMTU, &kReceiveMtu,
                   sizeof(kReceiveMtu)) < 0) {
      LOG(WARNING) << "Failed to set receive MTU on L2CAP server socket: "
                   << std::strerror(errno);
    }

    if (listen(server_fd_, 5) < 0) {
      LOG(ERROR) << "Failed to listen on L2CAP server socket: "
                 << std::strerror(errno);
      close(server_fd_);
      server_fd_ = -1;
      return nullptr;
    }

    socklen_t addr_len = sizeof(addr);
    if (getsockname(server_fd_, reinterpret_cast<sockaddr*>(&addr),
                    &addr_len) == 0) {
      psm_ = btohs(addr.l2_psm);
      LOG(INFO) << "L2CAP server socket listening on PSM: " << psm_;
    } else {
      LOG(WARNING) << "Failed to get socket name: " << std::strerror(errno);
    }
  }

  sockaddr_l2 client_addr;
  std::memset(&client_addr, 0, sizeof(client_addr));
  socklen_t client_len = sizeof(client_addr);

  LOG(INFO) << "Waiting for L2CAP connection on PSM " << psm_ << "...";

  int client_fd = accept(server_fd_, (struct sockaddr*) &client_addr, &client_len);

  if (client_fd < 0) {
    if (errno == EINTR || errno == EAGAIN) {
      LOG(INFO) << "Accept interrupted, returning nullptr";
      return nullptr;
    }
    LOG(ERROR) << "Failed to accept L2CAP connection: " << std::strerror(errno);
    return nullptr;
  }

  char client_addr_str[18];
  ba2str(&client_addr.l2_bdaddr, client_addr_str);
  LOG(INFO) << "Accepted L2CAP connection from " << client_addr_str
            << " on PSM " << btohs(client_addr.l2_psm);

  api::ble::BlePeripheral::UniqueId peripheral_id = 0;
  for (int i = 0; i < 6; ++i) {
    peripheral_id =
        (peripheral_id << 8) | static_cast<uint8_t>(client_addr.l2_bdaddr.b[i]);
  }

  std::string service_id;
  {
    absl::MutexLock lock(&mutex_);
    service_id = service_id_;
  }
  return std::make_unique<BleL2capSocket>(client_fd, peripheral_id, service_id );
}

Exception BleL2capServerSocket::Close() {
  int server_fd = -1;
  {
    absl::MutexLock lock(&mutex_);
    if (closed_) {
      return {Exception::kSuccess};
    }
    closed_ = true;
    server_fd = std::exchange(server_fd_, -1);
  }
  if (server_fd != -1 && close(server_fd) != 0) {
    LOG(WARNING) << "Failed to close L2CAP server socket: "
                 << std::strerror(errno);
  }
  return {Exception::kSuccess};
}

}  // namespace linux
}  // namespace nearby
