#include "sharing/linux/tui/ipc_client.h"

#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <cstring>
#include <utility>

namespace nearby::sharing::linux_tui {

IpcClient::IpcClient(std::string socket_path)
    : socket_path_(std::move(socket_path)) {}

IpcClient::~IpcClient() {
  Stop();
}

bool IpcClient::Start(EventHandler event_handler) {
  Stop();
  event_handler_ = std::move(event_handler);

  socket_fd_ = socket(AF_UNIX, SOCK_STREAM, 0);
  if (socket_fd_ < 0) {
    Emit({{"event", "ipc_disconnected"},
          {"message", "failed to create socket"}});
    return false;
  }

  sockaddr_un addr{};
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, socket_path_.c_str(), sizeof(addr.sun_path) - 1);

  if (connect(socket_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) <
      0) {
    close(socket_fd_);
    socket_fd_ = -1;
    Emit({{"event", "ipc_disconnected"},
          {"message", "failed to connect to daemon"}});
    return false;
  }

  running_.store(true);
  connected_.store(true);
  read_thread_ = std::thread([this] { ReadLoop(); });
  Emit({{"event", "ipc_connected"}});
  return true;
}

void IpcClient::Stop() {
  running_.store(false);
  connected_.store(false);

  {
    std::lock_guard<std::mutex> lock(write_mutex_);
    if (socket_fd_ >= 0) {
      shutdown(socket_fd_, SHUT_RDWR);
      close(socket_fd_);
      socket_fd_ = -1;
    }
  }

  if (read_thread_.joinable()) {
    read_thread_.join();
  }
}

bool IpcClient::Send(const nlohmann::json& message) {
  return SendLine(message.dump());
}

void IpcClient::ReadLoop() {
  std::string buffer;
  char chunk[1024]{};

  while (running_.load()) {
    ssize_t received = recv(socket_fd_, chunk, sizeof(chunk), 0);
    if (received > 0) {
      buffer.append(chunk, static_cast<size_t>(received));
      size_t newline = std::string::npos;
      while ((newline = buffer.find('\n')) != std::string::npos) {
        std::string line = buffer.substr(0, newline);
        buffer.erase(0, newline + 1);
        if (line.empty()) {
          continue;
        }
        try {
          Emit(nlohmann::json::parse(line));
        } catch (const nlohmann::json::exception& error) {
          Emit({{"event", "ipc_error"},
                {"message",
                 std::string("malformed daemon JSON: ") + error.what()}});
        }
      }
      continue;
    }

    if (received < 0 && errno == EINTR) {
      continue;
    }
    break;
  }

  connected_.store(false);
  if (running_.load()) {
    Emit({{"event", "ipc_disconnected"}, {"message", "daemon disconnected"}});
  }
}

void IpcClient::Emit(nlohmann::json event) {
  if (event_handler_) {
    event_handler_(event);
  }
}

bool IpcClient::SendLine(std::string_view line) {
  std::lock_guard<std::mutex> lock(write_mutex_);
  if (socket_fd_ < 0) {
    return false;
  }

  std::string data(line);
  if (data.empty() || data.back() != '\n') {
    data.push_back('\n');
  }

  size_t total_sent = 0;
  while (total_sent < data.size()) {
    ssize_t sent = send(socket_fd_, data.data() + total_sent,
                        data.size() - total_sent, MSG_NOSIGNAL);
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

}  // namespace nearby::sharing::linux_tui
