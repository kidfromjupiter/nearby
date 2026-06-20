#include <atomic>
#include <string>
#include <string_view>

#include <poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "absl/synchronization/mutex.h"
#include "gtest/gtest_prod.h"

constexpr std::string_view SOCK_PATH = "/tmp/nearby_sharing_sock";

class IPCServer {
 public:
  IPCServer() = default;

  ~IPCServer() { Stop(); }
  void Stop();
  void StartEventLoop();
  using Handler = std::function<void(std::string_view args)>;

  void RegisterHandler(std::string command, Handler handler) {
    absl::MutexLock lock(lock_);
    handlers_[std::move(command)] = std::move(handler);
  }

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
  std::unordered_map<std::string, Handler> handlers_;
  std::atomic<bool> running_{false};
};
