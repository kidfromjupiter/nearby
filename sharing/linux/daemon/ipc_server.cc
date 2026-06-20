#include "ipc_server.h"

#include <cerrno>
#include <cstring>
#include <iostream>
#include <thread>

namespace {

nlohmann::json CommandResult(std::string command, bool ok,
                             std::string message) {
  return nlohmann::json{{"event", "command_result"},
                        {"command", std::move(command)},
                        {"ok", ok},
                        {"message", std::move(message)}};
}

}  // namespace

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

  {
    absl::MutexLock write_lock(write_lock_);
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
  started_.store(true);
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

    {
      absl::MutexLock write_lock(write_lock_);
      client_fd_ = accepted_fd;
    }

    Recv();

    {
      absl::MutexLock write_lock(write_lock_);
      if (client_fd_ >= 0) {
        close(client_fd_);
        client_fd_ = -1;
      }
    }
  }

  Stop();
}

bool IPCServer::SendLine(std::string_view line) {
  absl::MutexLock write_lock(write_lock_);
  if (client_fd_ < 0) {
    return false;
  }

  std::string data(line);
  if (data.empty() || data.back() != '\n') {
    data.push_back('\n');
  }

  size_t total_sent = 0;
  while (total_sent < data.size()) {
    ssize_t sent =
        send(client_fd_, data.data() + total_sent, data.size() - total_sent, 0);
    if (sent > 0) {
      total_sent += static_cast<size_t>(sent);
      continue;
    }
    if (sent < 0 && errno == EINTR) {
      continue;
    }
    return false;
  }

  return true;
}

bool IPCServer::SendJson(const nlohmann::json& message) {
  return SendLine(message.dump());
}

void IPCServer::DispatchOne(const std::string& line) {
  nlohmann::json request;
  try {
    request = nlohmann::json::parse(line);
  } catch (const nlohmann::json::exception& e) {
    SendJson(CommandResult("", false, std::string("malformed JSON: ") + e.what()));
    return;
  }

  if (!request.is_object() || !request.contains("command") ||
      !request["command"].is_string()) {
    SendJson(CommandResult("", false, "missing string field: command"));
    return;
  }
  std::string command = request["command"].get<std::string>();

  Handler handler;

  {
    absl::MutexLock lock(lock_);

    auto it = handlers_.find(command);
    if (it == handlers_.end()) {
      SendJson(CommandResult(command, false, "unknown command"));
      return;
    }

    handler = it->second;
  }

  handler(request);
}
void IPCServer::DispatchLoop() {
  while (true) {
    std::string line = Read();

    if (line.empty()) {
      if (!running_.load() && started_.load()) {
        return;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      continue;
    }

    DispatchOne(line);
  }
}
