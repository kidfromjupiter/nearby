// Copyright 2023 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <cstring>

#include "internal/platform/implementation/linux/wifi_hotspot_server_socket.h"
#include "internal/platform/implementation/linux/wifi_hotspot_socket.h"
#include "internal/platform/implementation/linux/wifi_medium.h"
#include "internal/platform/service_address.h"
#include "internal/platform/wifi_credential.h"

namespace nearby {
namespace linux {
std::string NetworkManagerWifiHotspotServerSocket::GetIPAddress() const {
  auto ip4addresses = active_conn_->GetIP4Addresses();
  if (ip4addresses.empty()) {
    LOG(ERROR)
        << __func__
        << ": Could not find any IPv4 addresses for active connection "
        << active_conn_->getObjectPath();
    return {};
  }
  return ip4addresses[0];
}

int NetworkManagerWifiHotspotServerSocket::GetPort() const {
  struct sockaddr_in sin {};
  socklen_t len = sizeof(sin);
  auto ret =
      getsockname(fd_.get(), reinterpret_cast<struct sockaddr *>(&sin), &len);
  if (ret < 0) {
    LOG(ERROR) << __func__ << ": Error getting information for socket "
                       << fd_.get() << ": " << std::strerror(errno);
    return 0;
  }

  return ntohs(sin.sin_port);
}

std::unique_ptr<api::WifiHotspotSocket>
NetworkManagerWifiHotspotServerSocket::Accept() {
  struct sockaddr_in addr {};
  socklen_t len = sizeof(addr);

  // Poll with timeout to allow checking the closed flag periodically
  while (!closed_.load()) {
    struct pollfd pfd;
    pfd.fd = fd_.get();
    pfd.events = POLLIN;

    // Poll with 1 second timeout
    int poll_result = poll(&pfd, 1, 1000);
    
    if (poll_result < 0) {
      if (errno == EINTR) {
        continue;  // Interrupted, try again
      }
      LOG(ERROR) << __func__ << ": Error polling socket " << fd_.get() << ": "
                 << std::strerror(errno);
      return nullptr;
    }

    if (poll_result == 0) {
      // Timeout - check closed flag and continue
      continue;
    }

    // Data available, try to accept
    auto conn =
        accept(fd_.get(), reinterpret_cast<struct sockaddr *>(&addr), &len);
    if (conn < 0) {
      if (errno == EBADF || errno == EINVAL) {
        // Socket was closed
        return nullptr;
      }
      LOG(ERROR) << __func__
                 << ": Error accepting incoming connections on socket "
                 << fd_.get() << ": " << std::strerror(errno);
      return nullptr;
    }

    return std::make_unique<WifiHotspotSocket>(conn);
  }

  // Socket was closed
  return nullptr;
}

Exception NetworkManagerWifiHotspotServerSocket::Close() {
  closed_.store(true);
  int fd = fd_.release();
  shutdown(fd, SHUT_RDWR);
  auto ret = close(fd);
  if (ret < 0) {
    LOG(ERROR) << __func__
                       << ": Error closing socket: " << std::strerror(errno);
    return {Exception::kFailed};
  }

  return {Exception::kSuccess};
}

void NetworkManagerWifiHotspotServerSocket::PopulateHotspotCredentials(
    HotspotCredentials& hotspot_credentials) {
  // Get IPv4 addresses from the active hotspot connection
  auto ip4addresses = active_conn_->GetIP4Addresses();
  if (ip4addresses.empty()) {
    LOG(ERROR) << __func__
               << ": Could not find any IPv4 addresses for active connection "
               << active_conn_->getObjectPath();
    return;
  }

  // Get the server socket port
  uint16_t port = GetPort();
  if (port == 0) {
    LOG(ERROR) << __func__ << ": Invalid port number";
    return;
  }

  std::vector<ServiceAddress> service_addresses;

  // Convert each IP address to binary format and create ServiceAddress
  for (const auto& ip_str : ip4addresses) {
    struct in_addr addr;
    if (inet_pton(AF_INET, ip_str.c_str(), &addr) != 1) {
      LOG(ERROR) << __func__ << ": Invalid IPv4 address: " << ip_str;
      continue;
    }

    // Convert to vector of chars in network byte order
    std::vector<char> addr_bytes(4);
    std::memcpy(addr_bytes.data(), &addr.s_addr, 4);

    service_addresses.push_back(
        ServiceAddress{.address = std::move(addr_bytes), .port = port});
  }

  if (service_addresses.empty()) {
    LOG(ERROR) << __func__ << ": No valid IPv4 addresses found";
    return;
  }

  // Set the address candidates in the credentials
  hotspot_credentials.SetAddressCandidates(std::move(service_addresses));
}
}  // namespace linux
}  // namespace nearby
