#include <atomic>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>

#include <poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "absl/synchronization/mutex.h"
#include "gtest/gtest_prod.h"
#include "nlohmann/json.hpp"

constexpr std::string_view SOCK_PATH = "/tmp/nearby_sharing_sock";

class IPCServer {
 public:
  IPCServer() = default;

  ~IPCServer() { Stop(); }
  void Stop();
  void StartEventLoop();
  using Handler = std::function<void(const nlohmann::json& request)>;

  void RegisterHandler(std::string command, Handler handler) {
    absl::MutexLock lock(lock_);
    handlers_[std::move(command)] = std::move(handler);
  }

  bool SendLine(std::string_view line);
  bool SendJson(const nlohmann::json& message);
  void DispatchLoop();

 private:
  friend class IPCServerTest;

  void DispatchOne(const std::string& line);
  void InitialiseSock();
  void Recv();
  std::string Read();

  int sock_fd_ = -1;
  int client_fd_ = -1;
  sockaddr_un addr{};
  std::string read_buf;
  absl::Mutex lock_;
  absl::Mutex write_lock_;
  std::unordered_map<std::string, Handler> handlers_;
  std::atomic<bool> running_{false};
  std::atomic<bool> started_{false};
};
