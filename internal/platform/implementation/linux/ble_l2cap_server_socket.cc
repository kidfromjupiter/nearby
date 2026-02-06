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

#include <poll.h>
#include <fcntl.h>
#include <unistd.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#include <sys/socket.h>

#include <cerrno>
#include <cstring>

#include "internal/platform/logging.h"
#include "internal/platform/prng.h"

namespace nearby {
namespace linux {

bool SetNonBlocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags < 0) return false;
  return fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0;
}
void DrainFd(int fd) {
  char buf[64];
  for (;;) {
    ssize_t n = read(fd, buf, sizeof(buf));
    if (n > 0) continue;
    if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) break;
    break;
  }
}
BleL2capServerSocket::BleL2capServerSocket() : psm_(0) {}

BleL2capServerSocket::BleL2capServerSocket(int psm) : psm_(psm) {
}

BleL2capServerSocket::~BleL2capServerSocket() { Close(); }

void BleL2capServerSocket::SetPSM(int psm) { psm_ = psm; }

void BleL2capServerSocket::AcceptPoll(int& client_fd, sockaddr_l2& client_addr, socklen_t& client_len) {
  for (;;) {
    struct pollfd fds[2];
    fds[0].fd = server_fd_;
    fds[0].events = POLLIN;
    fds[0].revents = 0;

    fds[1].fd = stop_pipe_[0];
    fds[1].events = POLLIN;
    fds[1].revents = 0;

    int r = poll(fds, 2, -1);  // wait forever; stop pipe will wake us
    if (r < 0) {
      if (errno == EINTR) continue;
      LOG(ERROR) << "poll() failed: " << std::strerror(errno);
      return;
    }

    // Stop requested
if (fds[1].revents & POLLIN) {
  DrainFd(stop_pipe_[0]);
  client_fd = -1;
  errno = EINTR;              // optional: helps caller treat as "interrupted"
  return;                     // <-- THIS is the key
}

    // Listen socket error/hangup
    if (fds[0].revents & (POLLERR | POLLHUP | POLLNVAL)) {
      LOG(ERROR) << "poll() listen fd error revents=" << fds[0].revents;
    }

    // Incoming connection(s)
    if (fds[0].revents & POLLIN) {
      for (;;) {
        client_fd = accept(server_fd_, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd >= 0) break;

        if (errno == EAGAIN || errno == EWOULDBLOCK) {
          // No more queued connections; go back to poll().
          client_fd = -1;
          break;
        }
        if (errno == EINTR) continue;

        LOG(ERROR) << "Failed to accept L2CAP connection: " << std::strerror(errno);
        return;
      }

      if (client_fd >= 0) break;
    }
  }
}
std::unique_ptr<api::ble::BleL2capSocket> BleL2capServerSocket::Accept() {
    Prng prng;
    psm_ = 0x80 + (prng.NextUint32() % 0x80);

    server_fd_ = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
    if (server_fd_ < 0) {
      LOG(ERROR) << "Failed to create L2CAP server socket: "
                 << std::strerror(errno);
      return nullptr;
    }
    // Create stop pipe once (used to wake poll() from another thread).
    if (stop_pipe_[0] == -1 && stop_pipe_[1] == -1) {
      if (pipe(stop_pipe_) < 0) {
        LOG(ERROR) << "Failed to create stop pipe: " << std::strerror(errno);
        close(server_fd_);
        server_fd_ = -1;
        return nullptr;
      }
      // Optional, but avoids any chance of blocking on drain/write.
      (void)SetNonBlocking(stop_pipe_[0]);
      (void)SetNonBlocking(stop_pipe_[1]);
    }

    struct sockaddr_l2 addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.l2_family = AF_BLUETOOTH;
    addr.l2_psm = htobs(psm_);
    addr.l2_bdaddr_type = BDADDR_LE_PUBLIC;
    // Set BDADDR_ANY (all zeros)
    std::memset(&addr.l2_bdaddr, 0, sizeof(addr.l2_bdaddr));

    if (bind(server_fd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
      LOG(ERROR) << "Failed to bind L2CAP server socket: "
                 << std::strerror(errno) << " (errno: " << errno << ")";
      close(server_fd_);
      server_fd_ = -1;
      return nullptr;
    }

    struct l2cap_options opts;
    opts.omtu = 0;
    opts.imtu = 672;

    if (setsockopt(server_fd_, SOL_BLUETOOTH, BT_RCVMTU, &opts.imtu,
                sizeof(opts.imtu)) < 0) {
      LOG(ERROR) << "Failed to set socket options on L2CAP server socket" ;
                }

    if (listen(server_fd_, 5) < 0) {
      LOG(ERROR) << "Failed to listen on L2CAP server socket: "
                 << std::strerror(errno);
      close(server_fd_);
      server_fd_ = -1;
      return nullptr;
    }

    // Make accept() non-blocking; we will block in poll() instead.
    if (!SetNonBlocking(server_fd_)) {
      LOG(ERROR) << "Failed to set non-blocking on L2CAP server socket: " << std::strerror(errno);
      close(server_fd_);
      server_fd_ = -1;
      return nullptr;
    }
    socklen_t addr_len = sizeof(addr);
    if (getsockname(server_fd_, (struct sockaddr*)&addr, &addr_len) == 0) {
      psm_ = btohs(addr.l2_psm);
      LOG(INFO) << "L2CAP server socket listening on PSM: " << psm_;
    } else {
      LOG(ERROR) << "Failed to get socket name: " << std::strerror(errno);
    }

    if (server_fd_ < 0) {
      LOG(ERROR) << "Server socket not initialized";
      return nullptr;
  }

  struct sockaddr_l2 client_addr;
  socklen_t client_len = sizeof(client_addr);
  std::memset(&client_addr, 0, sizeof(client_addr));

  LOG(INFO) << "Waiting for L2CAP connection on PSM " << psm_ << "...";
  int client_fd = -1;
  AcceptPoll(client_fd, client_addr, client_len);

  if (client_fd < 0) {
    if (errno == EINTR || errno == EAGAIN) {
      LOG(WARNING) << "Accept interrupted, returning nullptr";
      return nullptr;
    }
    LOG(ERROR) << "Failed to accept L2CAP connection: " << std::strerror(errno);
    return nullptr;
  }

  char client_addr_str[18];
  ba2str(&client_addr.l2_bdaddr, client_addr_str);
  LOG(INFO) << "Accepted L2CAP connection from " << client_addr_str
            << " on PSM " << btohs(client_addr.l2_psm);

  LOG(INFO) << __func__ << ": Connected to client_fd: " << client_fd;
  // Create a unique ID from the MAC address
  api::ble::BlePeripheral::UniqueId peripheral_id = 0;
  for (int i = 0; i < 6; i++) {
    peripheral_id = (peripheral_id << 8) | client_addr.l2_bdaddr.b[i];
  }

  accepted_fds_.emplace(server_fd_, std::pair(client_fd, peripheral_id));
  return std::make_unique<BleL2capSocket>(client_fd, peripheral_id);
}
Exception BleL2capServerSocket::Close() {
  LOG(ERROR) << __func__ << ": closing l2cap server socket";

  if (stop_pipe_[1] != -1) (void)write(stop_pipe_[1], "x", 1);
  LOG(ERROR) << __func__ << ": l2cap server socket closed";
  return {Exception::kSuccess};
}


}  // namespace linux
}  // namespace nearby
