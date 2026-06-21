#pragma once

#include <atomic>
#include <functional>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>

#include "nlohmann/json.hpp"

namespace nearby::sharing::linux_tui {

class IpcClient {
 public:
  using EventHandler = std::function<void(const nlohmann::json& event)>;

  explicit IpcClient(std::string socket_path = "/tmp/nearby_sharing_sock");
  ~IpcClient();

  IpcClient(const IpcClient&) = delete;
  IpcClient& operator=(const IpcClient&) = delete;

  bool Start(EventHandler event_handler);
  void Stop();
  bool Send(const nlohmann::json& message);
  bool connected() const { return connected_.load(); }

 private:
  void ReadLoop();
  void Emit(nlohmann::json event);
  bool SendLine(std::string_view line);

  std::string socket_path_;
  EventHandler event_handler_;
  std::thread read_thread_;
  mutable std::mutex write_mutex_;
  std::atomic<bool> running_{false};
  std::atomic<bool> connected_{false};
  int socket_fd_ = -1;
};

}  // namespace nearby::sharing::linux_tui
