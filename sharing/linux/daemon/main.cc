#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string>
#include <stdio.h>
#include <string.h>
#include <errno.h>

constexpr std::string_view sock_path = "/tmp/nearby_sharing.sock";


int main() {
  // should always be waiting on the socket for new connections
  int retries = 3;
  while (retries >= 0) {
    int server_fd;
    struct sockaddr_un addr;

    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1) {
      retries--;
      continue;
    }

    // removes old socket
    unlink(sock_path.data());

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, sock_path.data(), sizeof(addr.sun_path) - 1);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
      close(server_fd);
      retries--;
      continue;
    }

    if (listen(server_fd, 10) == -1) {
      close(server_fd);
      retries--;
      continue;
    }
    socklen_t addr_len = sizeof(addr);

    // blocks till connection is present
    if (accept(server_fd, (struct sockaddr*)&addr, &addr_len)) {
      close(server_fd);
      retries--;
      continue;
    }
  }
}
