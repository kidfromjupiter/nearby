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

namespace nearby {
namespace linux {
namespace {

bool SetNonBlocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags < 0) return false;
  return fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0;
}

void DrainFd(int fd) {
  char buf[64];
  while (true) {
    ssize_t read_count = read(fd, buf, sizeof(buf));
    if (read_count > 0) continue;
    if (read_count < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) break;
    break;
  }
}

}  // namespace

BleL2capServerSocket::BleL2capServerSocket() = default;

BleL2capServerSocket::BleL2capServerSocket(
    int psm, BleL2capSocket::ProtocolMode protocol_mode, std::string service_id)
    : psm_(psm),
      protocol_mode_(protocol_mode),
      service_id_(std::move(service_id)) {}

BleL2capServerSocket::~BleL2capServerSocket() { Close(); }

void BleL2capServerSocket::SetPSM(int psm) {
  absl::MutexLock lock(&mutex_);
  psm_ = psm;
}

bool BleL2capServerSocket::InitializeServerSocketLocked() {
  if (closed_) {
    errno = EINTR;
    return false;
  }

  if (server_fd_ >= 0) {
    return true;
  }

  if ((stop_pipe_[0] == -1) != (stop_pipe_[1] == -1)) {
    if (stop_pipe_[0] != -1) close(stop_pipe_[0]);
    if (stop_pipe_[1] != -1) close(stop_pipe_[1]);
    stop_pipe_[0] = -1;
    stop_pipe_[1] = -1;
  }

  if (stop_pipe_[0] == -1) {
    if (pipe(stop_pipe_) < 0) {
      LOG(ERROR) << "Failed to create stop pipe: " << std::strerror(errno);
      return false;
    }
    if (!SetNonBlocking(stop_pipe_[0]) || !SetNonBlocking(stop_pipe_[1])) {
      LOG(ERROR) << "Failed to set non-blocking mode on stop pipe: "
                 << std::strerror(errno);
      close(stop_pipe_[0]);
      close(stop_pipe_[1]);
      stop_pipe_[0] = -1;
      stop_pipe_[1] = -1;
      return false;
    }
  }

  server_fd_ = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
  if (server_fd_ < 0) {
    LOG(ERROR) << "Failed to create L2CAP server socket: "
               << std::strerror(errno);
    return false;
  }

  sockaddr_l2 addr;
  std::memset(&addr, 0, sizeof(addr));
  addr.l2_family = AF_BLUETOOTH;
  addr.l2_psm = htobs(psm_);
  addr.l2_bdaddr_type = BDADDR_LE_PUBLIC;
  std::memset(&addr.l2_bdaddr, 0, sizeof(addr.l2_bdaddr));

  if (bind(server_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
    LOG(ERROR) << "Failed to bind L2CAP server socket: " << std::strerror(errno)
               << " (errno: " << errno << ")";
    close(server_fd_);
    server_fd_ = -1;
    return false;
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
    return false;
  }

  if (!SetNonBlocking(server_fd_)) {
    LOG(ERROR) << "Failed to set non-blocking on L2CAP server socket: "
               << std::strerror(errno);
    close(server_fd_);
    server_fd_ = -1;
    return false;
  }

  socklen_t addr_len = sizeof(addr);
  if (getsockname(server_fd_, reinterpret_cast<sockaddr*>(&addr), &addr_len) ==
      0) {
    psm_ = btohs(addr.l2_psm);
    LOG(INFO) << "L2CAP server socket listening on PSM: " << psm_;
  } else {
    LOG(WARNING) << "Failed to get socket name: " << std::strerror(errno);
  }

  return true;
}

void BleL2capServerSocket::AcceptPoll(int server_fd, int stop_fd, int& client_fd,
                                      sockaddr_l2& client_addr,
                                      socklen_t& client_len) {
  while (true) {
    pollfd fds[2];
    fds[0].fd = server_fd;
    fds[0].events = POLLIN;
    fds[0].revents = 0;
    fds[1].fd = stop_fd;
    fds[1].events = POLLIN;
    fds[1].revents = 0;

    int result = poll(fds, 2, -1);
    if (result < 0) {
      if (errno == EINTR) continue;
      LOG(ERROR) << "poll() failed: " << std::strerror(errno);
      client_fd = -1;
      return;
    }

    if (fds[1].revents & (POLLIN | POLLERR | POLLHUP | POLLNVAL)) {
      if (fds[1].revents & POLLIN) {
        DrainFd(stop_fd);
      }
      client_fd = -1;
      errno = EINTR;
      return;
    }

    if (fds[0].revents & (POLLERR | POLLHUP | POLLNVAL)) {
      LOG(ERROR) << "poll() listen fd error revents=" << fds[0].revents;
      client_fd = -1;
      errno = EIO;
      return;
    }

    if ((fds[0].revents & POLLIN) == 0) continue;

    while (true) {
      client_fd = accept(server_fd, reinterpret_cast<sockaddr*>(&client_addr),
                         &client_len);
      if (client_fd >= 0) {
        return;
      }

      if (errno == EINTR) continue;
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        client_fd = -1;
        break;
      }

      LOG(ERROR) << "Failed to accept L2CAP connection: " << std::strerror(errno);
      return;
    }
  }
}

std::unique_ptr<api::ble::BleL2capSocket> BleL2capServerSocket::Accept() {
  int server_fd = -1;
  int stop_fd = -1;
  int listening_psm = 0;
  {
    absl::MutexLock lock(&mutex_);
    if (!InitializeServerSocketLocked()) return nullptr;
    server_fd = server_fd_;
    stop_fd = stop_pipe_[0];
    listening_psm = psm_;
  }

  sockaddr_l2 client_addr;
  std::memset(&client_addr, 0, sizeof(client_addr));
  socklen_t client_len = sizeof(client_addr);

  LOG(INFO) << "Waiting for L2CAP connection on PSM " << listening_psm << "...";
  int client_fd = -1;
  AcceptPoll(server_fd, stop_fd, client_fd, client_addr, client_len);

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
        (peripheral_id << 8) |
        static_cast<uint8_t>(client_addr.l2_bdaddr.b[i]);
  }

  BleL2capSocket::ProtocolMode protocol_mode;
  std::string service_id;
  {
    absl::MutexLock lock(&mutex_);
    protocol_mode = protocol_mode_;
    service_id = service_id_;
  }
  return std::make_unique<BleL2capSocket>(
      client_fd, peripheral_id, protocol_mode, service_id,
      /*incoming_connection=*/true);
}

Exception BleL2capServerSocket::Close() {
  absl::AnyInvocable<void()> notifier;
  int server_fd = -1;
  int stop_read_fd = -1;
  int stop_write_fd = -1;
  {
    absl::MutexLock lock(&mutex_);
    if (closed_) {
      return {Exception::kSuccess};
    }
    closed_ = true;
    notifier = std::move(close_notifier_);
    server_fd = std::exchange(server_fd_, -1);
    stop_read_fd = std::exchange(stop_pipe_[0], -1);
    stop_write_fd = std::exchange(stop_pipe_[1], -1);
  }

  if (stop_write_fd != -1) {
    char wake = 'x';
    ssize_t ignored = write(stop_write_fd, &wake, 1);
    (void)ignored;
  }

  if (server_fd != -1 && close(server_fd) != 0) {
    LOG(WARNING) << "Failed to close L2CAP server socket: " << std::strerror(errno);
  }
  if (stop_read_fd != -1 && close(stop_read_fd) != 0) {
    LOG(WARNING) << "Failed to close stop pipe read fd: " << std::strerror(errno);
  }
  if (stop_write_fd != -1 && close(stop_write_fd) != 0) {
    LOG(WARNING) << "Failed to close stop pipe write fd: "
                 << std::strerror(errno);
  }

  if (notifier) {
    notifier();
  }

  return {Exception::kSuccess};
}

void BleL2capServerSocket::SetCloseNotifier(
    absl::AnyInvocable<void()> notifier) {
  absl::AnyInvocable<void()> notifier_to_run;
  {
    absl::MutexLock lock(&mutex_);
    if (!closed_) {
      close_notifier_ = std::move(notifier);
      return;
    }
    notifier_to_run = std::move(notifier);
  }
  if (notifier_to_run) {
    notifier_to_run();
  }
}

}  // namespace linux
}  // namespace nearby
