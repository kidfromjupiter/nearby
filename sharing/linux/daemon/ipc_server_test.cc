#include <gtest/gtest.h>

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <cstring>
#include <fstream>
#include <functional>
#include <string>
#include <string_view>
#include <thread>

#include "ipc_server.h"

class IPCServerTest : public ::testing::Test {
 protected:
  static std::string Read(IPCServer& server) {
    return server.Read();
  }

  static void DispatchOne(IPCServer& server, const std::string& line) {
    server.DispatchOne(line);
  }

  static void SetReadBuffer(IPCServer& server, std::string value) {
    absl::MutexLock lock(server.lock_);
    server.read_buf = std::move(value);
  }

  static void SetRunning(IPCServer& server, bool running) {
    server.running_.store(running);
    server.started_.store(true);
  }

  static bool IsRunning(IPCServer& server) {
    return server.running_.load();
  }

  static std::string WaitRead(IPCServer& server) {
    for (int i = 0; i < 100; ++i) {
      std::string cmd = Read(server);

      if (!cmd.empty()) {
        return cmd;
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return "";
  }

  static bool WaitUntilTrue(const std::function<bool()>& condition) {
    for (int i = 0; i < 100; ++i) {
      if (condition()) {
        return true;
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return false;
  }
};

namespace {

int ConnectClientWithRetry() {
  int client_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  EXPECT_GE(client_fd, 0);

  sockaddr_un addr {};
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, SOCK_PATH.data(), sizeof(addr.sun_path) - 1);

  for (int i = 0; i < 100; ++i) {
    int result = connect(
        client_fd,
        reinterpret_cast<sockaddr*>(&addr),
        sizeof(addr));

    if (result == 0) {
      return client_fd;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  ADD_FAILURE() << "connect failed: " << strerror(errno);
  close(client_fd);
  return -1;
}

void SendAll(int fd, std::string_view data) {
  size_t total_sent = 0;

  while (total_sent < data.size()) {
    ssize_t sent = send(
        fd,
        data.data() + total_sent,
        data.size() - total_sent,
        0);

    ASSERT_GT(sent, 0) << "send failed: " << strerror(errno);

    total_sent += static_cast<size_t>(sent);
  }
}

void StopAndJoin(IPCServer& server, std::thread& server_thread) {
  server.Stop();

  if (server_thread.joinable()) {
    server_thread.join();
  }

  unlink(SOCK_PATH.data());
}

}  // namespace

TEST_F(IPCServerTest, ServerCanReadMultipleCommands) {
  IPCServer server;

  std::thread server_thread([&server]() {
    server.StartEventLoop();
  });

  int client_fd = ConnectClientWithRetry();
  ASSERT_GE(client_fd, 0);

  SendAll(client_fd, "A\nB\nC\n");

  EXPECT_EQ(WaitRead(server), "A");
  EXPECT_EQ(WaitRead(server), "B");
  EXPECT_EQ(WaitRead(server), "C");

  close(client_fd);
  StopAndJoin(server, server_thread);
}

TEST_F(IPCServerTest, PartialCommandIsBufferedUntilDelimiter) {
  IPCServer server;

  std::thread server_thread([&server]() {
    server.StartEventLoop();
  });

  int client_fd = ConnectClientWithRetry();
  ASSERT_GE(client_fd, 0);

  SendAll(client_fd, "PIN");

  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  EXPECT_EQ(Read(server), "");

  SendAll(client_fd, "G\n");

  EXPECT_EQ(WaitRead(server), "PING");

  close(client_fd);
  StopAndJoin(server, server_thread);
}

TEST_F(IPCServerTest, LargeCommandAcrossMultipleRecvCalls) {
  IPCServer server;

  std::thread server_thread([&server]() {
    server.StartEventLoop();
  });

  int client_fd = ConnectClientWithRetry();
  ASSERT_GE(client_fd, 0);

  std::string large(8192, 'A');
  SendAll(client_fd, large + "\n");

  EXPECT_EQ(WaitRead(server), large);

  close(client_fd);
  StopAndJoin(server, server_thread);
}

TEST_F(IPCServerTest, MultipleCommandsSplitAcrossSends) {
  IPCServer server;

  std::thread server_thread([&server]() {
    server.StartEventLoop();
  });

  int client_fd = ConnectClientWithRetry();
  ASSERT_GE(client_fd, 0);

  SendAll(client_fd, "A\nB");
  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  SendAll(client_fd, "\nC\n");

  EXPECT_EQ(WaitRead(server), "A");
  EXPECT_EQ(WaitRead(server), "B");
  EXPECT_EQ(WaitRead(server), "C");

  close(client_fd);
  StopAndJoin(server, server_thread);
}

TEST_F(IPCServerTest, ClientDisconnectDoesNotCrashServer) {
  IPCServer server;

  std::thread server_thread([&server]() {
    server.StartEventLoop();
  });

  int client_fd = ConnectClientWithRetry();
  ASSERT_GE(client_fd, 0);

  close(client_fd);

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  StopAndJoin(server, server_thread);

  SUCCEED();
}

TEST_F(IPCServerTest, ClientCanReconnectAfterDisconnect) {
  IPCServer server;

  std::thread server_thread([&server]() {
    server.StartEventLoop();
  });

  int client1_fd = ConnectClientWithRetry();
  ASSERT_GE(client1_fd, 0);

  SendAll(client1_fd, "FIRST\n");
  EXPECT_EQ(WaitRead(server), "FIRST");

  close(client1_fd);

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  int client2_fd = ConnectClientWithRetry();
  ASSERT_GE(client2_fd, 0);

  SendAll(client2_fd, "SECOND\n");
  EXPECT_EQ(WaitRead(server), "SECOND");

  close(client2_fd);
  StopAndJoin(server, server_thread);
}

TEST_F(IPCServerTest, StopEndsEventLoopThread) {
  IPCServer server;

  std::thread server_thread([&server]() {
    server.StartEventLoop();
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  StopAndJoin(server, server_thread);

  SUCCEED();
}

TEST_F(IPCServerTest, StopWhileClientConnected) {
  IPCServer server;

  std::thread server_thread([&server]() {
    server.StartEventLoop();
  });

  int client_fd = ConnectClientWithRetry();
  ASSERT_GE(client_fd, 0);

  SendAll(client_fd, "PING\n");
  EXPECT_EQ(WaitRead(server), "PING");

  StopAndJoin(server, server_thread);

  close(client_fd);

  SUCCEED();
}

TEST_F(IPCServerTest, StaleSocketPathIsCleanedUp) {
  unlink(SOCK_PATH.data());

  {
    std::ofstream stale_file{SOCK_PATH.data()};
    ASSERT_TRUE(stale_file.is_open());
    stale_file << "stale";
  }

  struct stat before {};
  ASSERT_EQ(stat(SOCK_PATH.data(), &before), 0);
  ASSERT_TRUE(S_ISREG(before.st_mode));

  IPCServer server;

  std::thread server_thread([&server]() {
    server.StartEventLoop();
  });

  int client_fd = ConnectClientWithRetry();
  ASSERT_GE(client_fd, 0);

  struct stat after {};
  ASSERT_EQ(stat(SOCK_PATH.data(), &after), 0);
  EXPECT_TRUE(S_ISSOCK(after.st_mode));

  close(client_fd);
  StopAndJoin(server, server_thread);
}

TEST_F(IPCServerTest, DispatchOneCallsRegisteredHandler) {
  IPCServer server;

  bool called = false;

  server.RegisterHandler("PING", [&](const nlohmann::json& request) {
    EXPECT_EQ(request.value("command", ""), "PING");
    called = true;
  });

  DispatchOne(server, R"({"command":"PING"})");

  EXPECT_TRUE(called);
}

TEST_F(IPCServerTest, DispatchOnePassesArguments) {
  IPCServer server;

  std::string received_args;

  server.RegisterHandler("ECHO", [&](const nlohmann::json& request) {
    received_args = request.value("args", "");
  });

  DispatchOne(server, R"({"command":"ECHO","args":"hello world"})");

  EXPECT_EQ(received_args, "hello world");
}

TEST_F(IPCServerTest, DispatchOneHandlesCommandWithoutArgs) {
  IPCServer server;

  std::string received_args = "not empty";

  server.RegisterHandler("PING", [&](const nlohmann::json& request) {
    received_args = request.value("args", "");
  });

  DispatchOne(server, R"({"command":"PING"})");

  EXPECT_EQ(received_args, "");
}

TEST_F(IPCServerTest, DispatchOneIgnoresUnknownCommand) {
  IPCServer server;

  bool called = false;

  server.RegisterHandler("PING", [&](const nlohmann::json& request) {
    called = true;
  });

  DispatchOne(server, R"({"command":"UNKNOWN","args":"something"})");

  EXPECT_FALSE(called);
}

TEST_F(IPCServerTest, DispatchLoopDispatchesBufferedCommand) {
  IPCServer server;

  std::atomic<bool> called{false};
  std::string received_args;

  server.RegisterHandler("ECHO", [&](const nlohmann::json& request) {
    received_args = request.value("args", "");
    called.store(true);
  });

  SetReadBuffer(server, R"({"command":"ECHO","args":"hello"})"
                            "\n");
  SetRunning(server, true);

  std::thread dispatch_thread([&server]() {
    server.DispatchLoop();
  });

  ASSERT_TRUE(WaitUntilTrue([&]() {
    return called.load();
  }));

  SetRunning(server, false);

  dispatch_thread.join();

  EXPECT_EQ(received_args, "hello");
}

TEST_F(IPCServerTest, SocketCommandReachesRegisteredHandler) {
  IPCServer server;

  std::atomic<bool> called{false};
  std::string received_args;

  server.RegisterHandler("ECHO", [&](const nlohmann::json& request) {
    received_args = request.value("args", "");
    called.store(true);
  });

  std::thread server_thread([&server]() {
    server.StartEventLoop();
  });

  int client_fd = ConnectClientWithRetry();
  ASSERT_GE(client_fd, 0);

  std::thread dispatch_thread([&server]() {
    server.DispatchLoop();
  });

  SendAll(client_fd, R"({"command":"ECHO","args":"from socket"})"
                     "\n");

  ASSERT_TRUE(WaitUntilTrue([&]() {
    return called.load();
  }));

  EXPECT_EQ(received_args, "from socket");

  close(client_fd);

  server.Stop();

  if (server_thread.joinable()) {
    server_thread.join();
  }

  if (dispatch_thread.joinable()) {
    dispatch_thread.join();
  }

  unlink(SOCK_PATH.data());
}
