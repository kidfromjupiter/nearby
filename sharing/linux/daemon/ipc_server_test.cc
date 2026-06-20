#include <gtest/gtest.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <chrono>
#include <cstring>
#include <string>
#include <fstream>
#include <thread>

#include "ipc_server.h"

namespace {


int ConnectClientWithRetry() {
  int client_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  EXPECT_GE(client_fd, 0);

  sockaddr_un addr{};
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

std::string WaitRead(IPCServer& server) {
  for (int i = 0; i < 100; ++i) {
    std::string cmd = server.Read();

    if (!cmd.empty()) {
      return cmd;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  return "";
}


void StopAndJoin(IPCServer& server, std::thread& server_thread) {
  server.Stop();

  if (server_thread.joinable()) {
    server_thread.join();
  }

  unlink(SOCK_PATH.data());
}

TEST(IPCServerEventLoopTest, ServerCanReadMultipleCommands) {
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

TEST(IPCServerEventLoopTest, PartialCommandIsBufferedUntilDelimiter) {
  IPCServer server;

  std::thread server_thread([&server]() {
    server.StartEventLoop();
  });

  int client_fd = ConnectClientWithRetry();
  ASSERT_GE(client_fd, 0);

  SendAll(client_fd, "PIN");

  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  EXPECT_EQ(server.Read(), "");

  SendAll(client_fd, "G\n");

  EXPECT_EQ(WaitRead(server), "PING");

  close(client_fd);
  StopAndJoin(server, server_thread);
}

TEST(IPCServerEventLoopTest, LargeCommandAcrossMultipleRecvCalls) {
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

TEST(IPCServerEventLoopTest, MultipleCommandsSplitAcrossSends) {
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

TEST(IPCServerEventLoopTest, ClientDisconnectDoesNotCrashServer) {
  IPCServer server;

  std::thread server_thread([&server]() {
    server.StartEventLoop();
  });

  int client_fd = ConnectClientWithRetry();
  ASSERT_GE(client_fd, 0);

  close(client_fd);

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Main expectation: server did not crash/hang.
  StopAndJoin(server, server_thread);

  SUCCEED();
}

TEST(IPCServerEventLoopTest, ClientCanReconnectAfterDisconnect) {
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

TEST(IPCServerEventLoopTest, StopEndsEventLoopThread) {
  IPCServer server;

  std::thread server_thread([&server]() {
    server.StartEventLoop();
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  StopAndJoin(server, server_thread);

  SUCCEED();
}

TEST(IPCServerEventLoopTest, StopWhileClientConnected) {
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

TEST(IPCServerEventLoopTest, StaleSocketPathIsCleanedUp) {
  unlink(SOCK_PATH.data());

  {
    std::ofstream stale_file{SOCK_PATH.data()};
    stale_file << "stale";
  }

  struct stat before{};
  ASSERT_EQ(stat(SOCK_PATH.data(), &before), 0);
  ASSERT_TRUE(S_ISREG(before.st_mode));

  IPCServer server;

  std::thread server_thread([&server]() {
    server.StartEventLoop();
  });

  int client_fd = ConnectClientWithRetry();
  ASSERT_GE(client_fd, 0);

  struct stat after{};
  ASSERT_EQ(stat(SOCK_PATH.data(), &after), 0);
  EXPECT_TRUE(S_ISSOCK(after.st_mode));

  close(client_fd);
  StopAndJoin(server, server_thread);
}
}  // namespace
