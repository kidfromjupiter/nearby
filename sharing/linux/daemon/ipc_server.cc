#include "ipc_server.h"
#include <thread>
#include <cerrno>
#include <cstring>
#include <iostream>

std::string IPCServer::Read() {
  std::string cmd;
  absl::MutexLock lock(lock_);

  size_t pos = read_buf.find('\n');

  if (pos != std::string::npos) {
    cmd = read_buf.substr(0, pos);
    read_buf.erase(0, pos + 1);
  }

  return cmd;
}

void IPCServer::Stop() {
  running_.store(false);

  if (client_fd_ >= 0) {
    shutdown(client_fd_, SHUT_RDWR);
    close(client_fd_);
    client_fd_ = -1;
  }

  if (sock_fd_ >= 0) {
    shutdown(sock_fd_, SHUT_RDWR);
    close(sock_fd_);
    sock_fd_ = -1;
  }

  unlink(SOCK_PATH.data());
}

void IPCServer::InitialiseSock() {
  sock_fd_ = socket(AF_UNIX, SOCK_STREAM, 0);
  if (sock_fd_ == -1) {
    return;
  }

  unlink(SOCK_PATH.data());

  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, SOCK_PATH.data(), sizeof(addr.sun_path) - 1);

  if (bind(sock_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1) {
    close(sock_fd_);
    sock_fd_ = -1;
    return;
  }

  if (listen(sock_fd_, 1) == -1) {
    close(sock_fd_);
    sock_fd_ = -1;
    return;
  }
}
void IPCServer::Recv() {
  while (running_.load()) {
    pollfd pfd{};
    pfd.fd = client_fd_;
    pfd.events = POLLIN;

    int poll_result = poll(&pfd, 1, 100);

    if (!running_.load()) {
      return;
    }

    if (poll_result < 0) {
      if (errno == EINTR) {
        continue;
      }
      return;
    }

    if (poll_result == 0) {
      continue;
    }

    if (pfd.revents & (POLLNVAL | POLLERR)) {
      return;
    }

    if (pfd.revents & (POLLIN | POLLHUP)) {
      char buf[1024]{};

      ssize_t n = recv(client_fd_, buf, sizeof(buf), 0);

      if (n > 0) {
        absl::MutexLock lock(lock_);
        read_buf.append(buf, static_cast<size_t>(n));
        continue;
      }

      if (n == 0) {
        return;
      }

      if (errno == EINTR) {
        continue;
      }

      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        continue;
      }

      return;
    }
  }
}

void IPCServer::StartEventLoop() {
  running_.store(true);

  InitialiseSock();

  if (sock_fd_ < 0) {
    running_.store(false);
    return;
  }

  while (running_.load()) {
    sockaddr_un client_addr{};
    socklen_t addr_len = sizeof(client_addr);

    int accepted_fd =
        accept(sock_fd_, reinterpret_cast<sockaddr*>(&client_addr), &addr_len);

    if (!running_.load()) {
      if (accepted_fd >= 0) {
        close(accepted_fd);
      }
      break;
    }

    if (accepted_fd < 0) {
      if (errno == EINTR) {
        continue;
      }
      break;
    }

    client_fd_ = accepted_fd;

    Recv();

    if (client_fd_ >= 0) {
      close(client_fd_);
      client_fd_ = -1;
    }
  }

  Stop();
}
