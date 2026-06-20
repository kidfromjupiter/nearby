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
  void InitialiseSock();
  void Recv();
  void StartEventLoop();
  std::string Read();

 private:
  FRIEND_TEST(IPCServerEventLoopTest, ClientCanConnectSendAndServerCanRead);
  FRIEND_TEST(IPCServerEventLoopTest, ServerCanReadMultipleCommands);
  FRIEND_TEST(IPCServerEventLoopTest, PartialCommandIsBufferedUntilDelimiter);
  FRIEND_TEST(IPCServerEventLoopTest, LargeCommandAcrossMultipleRecvCalls);
  FRIEND_TEST(IPCServerEventLoopTest, MultipleCommandsSplitAcrossSends);
  FRIEND_TEST(IPCServerEventLoopTest, ClientDisconnectDoesNotCrashServer);
  FRIEND_TEST(IPCServerEventLoopTest, ClientCanReconnectAfterDisconnect);
  FRIEND_TEST(IPCServerEventLoopTest, StopEndsEventLoopThread);
  FRIEND_TEST(IPCServerEventLoopTest, StopWhileClientConnected);
  FRIEND_TEST(IPCServerEventLoopTest, StaleSocketPathIsCleanedUp);

  int sock_fd_ = -1;
  int client_fd_ = -1;
  sockaddr_un addr{};
  std::string read_buf;
  absl::Mutex lock_;
  std::atomic<bool> running_{false};
};
