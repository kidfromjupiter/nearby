#include "sharing/linux/tui/ipc_client.h"

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <condition_variable>
#include <chrono>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "gtest/gtest.h"
#include "nlohmann/json.hpp"

namespace nearby::sharing::linux_tui {
namespace {

std::string TestSocketPath() {
  return "/tmp/nearby_tui_ipc_client_test_" + std::to_string(getpid());
}

int CreateServerSocket(const std::string& socket_path) {
  int fd = socket(AF_UNIX, SOCK_STREAM, 0);
  EXPECT_GE(fd, 0);

  unlink(socket_path.c_str());

  sockaddr_un addr{};
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, socket_path.c_str(), sizeof(addr.sun_path) - 1);

  EXPECT_EQ(bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)), 0);
  EXPECT_EQ(listen(fd, 1), 0);
  return fd;
}

std::string ReadLine(int fd) {
  std::string line;
  char byte = '\0';
  while (recv(fd, &byte, 1, 0) == 1) {
    if (byte == '\n') {
      return line;
    }
    line.push_back(byte);
  }
  return line;
}

void SendAll(int fd, const std::string& data) {
  size_t sent_total = 0;
  while (sent_total < data.size()) {
    ssize_t sent = send(fd, data.data() + sent_total, data.size() - sent_total,
                        MSG_NOSIGNAL);
    ASSERT_GT(sent, 0);
    sent_total += static_cast<size_t>(sent);
  }
}

}  // namespace

TEST(IpcClientTest, SendsJsonCommandWithNewline) {
  std::string socket_path = TestSocketPath();
  int server_fd = CreateServerSocket(socket_path);
  std::string received;

  std::thread server_thread([&] {
    int client_fd = accept(server_fd, nullptr, nullptr);
    ASSERT_GE(client_fd, 0);
    received = ReadLine(client_fd);
    close(client_fd);
  });

  IpcClient client(socket_path);
  ASSERT_TRUE(client.Start([](const nlohmann::json&) {}));
  EXPECT_TRUE(client.Send({{"command", "start_receive"}}));
  client.Stop();

  server_thread.join();
  close(server_fd);
  unlink(socket_path.c_str());

  nlohmann::json command = nlohmann::json::parse(received);
  EXPECT_EQ(command["command"], "start_receive");
}

TEST(IpcClientTest, ParsesSplitJsonEvents) {
  std::string socket_path = TestSocketPath();
  int server_fd = CreateServerSocket(socket_path);

  std::mutex mutex;
  std::condition_variable cv;
  std::vector<nlohmann::json> events;

  std::thread server_thread([&] {
    int client_fd = accept(server_fd, nullptr, nullptr);
    ASSERT_GE(client_fd, 0);
    SendAll(client_fd,
            R"({"event":"target_discovered","share_target":{"id":1}})"
            "\n"
            R"({"event":"incoming)");
    SendAll(client_fd, R"(_transfer","share_target":{"id":2},"transfer":{}})"
                       "\n");
    close(client_fd);
  });

  IpcClient client(socket_path);
  ASSERT_TRUE(client.Start([&](const nlohmann::json& event) {
    if (event.value("event", std::string()).rfind("ipc_", 0) == 0) {
      return;
    }
    std::lock_guard<std::mutex> lock(mutex);
    events.push_back(event);
    cv.notify_one();
  }));

  {
    std::unique_lock<std::mutex> lock(mutex);
    cv.wait_for(lock, std::chrono::seconds(2),
                [&] { return events.size() >= 2; });
  }
  client.Stop();

  server_thread.join();
  close(server_fd);
  unlink(socket_path.c_str());

  ASSERT_GE(events.size(), 2u);
  EXPECT_EQ(events[0]["event"], "target_discovered");
  EXPECT_EQ(events[1]["event"], "incoming_transfer");
}

}  // namespace nearby::sharing::linux_tui
