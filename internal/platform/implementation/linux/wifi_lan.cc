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
#include <sys/socket.h>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <memory>
#include <utility>

#include <sdbus-c++/Error.h>
#include <sdbus-c++/IConnection.h>
#include <sdbus-c++/Types.h>

#include "absl/strings/substitute.h"
#include "internal/platform/implementation/linux/avahi.h"
#include "internal/platform/implementation/linux/dbus.h"
#include "internal/platform/implementation/linux/tcp_server_socket.h"
#include "internal/platform/implementation/linux/wifi_lan.h"
#include "internal/platform/implementation/linux/wifi_lan_server_socket.h"
#include "internal/platform/implementation/linux/wifi_lan_socket.h"
#include "internal/platform/implementation/wifi_lan.h"
#include "internal/platform/logging.h"

namespace nearby {
namespace linux {
namespace {
constexpr char kDeviceIpv4TxtRecord[] = "IPv4";

std::string GetActiveIpv4Address(
    const std::shared_ptr<networkmanager::NetworkManager> &network_manager,
    const std::shared_ptr<sdbus::IConnection> &system_bus) {
  std::vector<sdbus::ObjectPath> connection_paths;
  try {
    connection_paths = network_manager->ActiveConnections();
  } catch (const sdbus::Error &e) {
    DBUS_LOG_PROPERTY_GET_ERROR(network_manager, "ActiveConnections", e);
    return {};
  }

  for (auto &path : connection_paths) {
    auto active_connection =
        std::make_unique<networkmanager::ActiveConnection>(system_bus, path);
    std::string conn_type;
    try {
      conn_type = active_connection->Type();
    } catch (const sdbus::Error &e) {
      DBUS_LOG_PROPERTY_GET_ERROR(active_connection, "Type", e);
      continue;
    }
    if (conn_type == "802-11-wireless" || conn_type == "802-3-ethernet") {
      auto ip4addresses = active_connection->GetIP4Addresses();
      if (!ip4addresses.empty()) {
        return ip4addresses[0];
      }
    }
  }

  return {};
}
}  // namespace

WifiLanMedium::WifiLanMedium(
    std::shared_ptr<networkmanager::NetworkManager> network_manager)
    : system_bus_(network_manager->GetConnection()),
      network_manager_(std::move(network_manager)),
      avahi_(std::make_shared<avahi::Server>(*system_bus_)) {}

bool WifiLanMedium::IsNetworkConnected() const {
  auto state = network_manager_->getState();
  return state == networkmanager::NetworkManager::kNMStateConnectedLocal ||
         state == networkmanager::NetworkManager::kNMStateConnectedSite ||
         state == networkmanager::NetworkManager::kNMStateConnectedGlobal;
}

std::optional<std::pair<std::string, std::string>> entry_group_key(
    const NsdServiceInfo &nsd_service_info) {
  auto name = nsd_service_info.GetServiceName();
  if (name.empty()) {
    LOG(ERROR) << __func__ << ": service name cannot be empty";
    return std::nullopt;
  }

  auto type = nsd_service_info.GetServiceType();
  if (type.empty()) {
    LOG(ERROR) << __func__ << ": service type cannot be empty";
    return std::nullopt;
  }

  return std::make_pair(std::move(name), std::move(type));
}

bool WifiLanMedium::StartAdvertising(const NsdServiceInfo &nsd_service_info) {
  auto key = entry_group_key(nsd_service_info);
  if (!key.has_value()) {
    return false;
  }

  {
    absl::ReaderMutexLock l(&entry_groups_mutex_);
    if (entry_groups_.count(*key) == 1) {
      LOG(ERROR) << __func__
                         << ": advertising is already active for this service";
      return false;
    }
  }

  auto txt_records_map = nsd_service_info.GetTxtRecords();
  if (txt_records_map.find(kDeviceIpv4TxtRecord) == txt_records_map.end()) {
    std::string ip_address =
        GetActiveIpv4Address(network_manager_, system_bus_);
    if (!ip_address.empty()) {
      txt_records_map.insert_or_assign(kDeviceIpv4TxtRecord, ip_address);
    }
  }
  std::vector<std::vector<std::uint8_t>> txt_records(txt_records_map.size());
  std::size_t i = 0;

  for (auto [key, value] : txt_records_map) {
    std::string entry = absl::Substitute("$0=$1", key, value);
    txt_records[i++] = std::vector<std::uint8_t>(entry.begin(), entry.end());
  }

  sdbus::ObjectPath entry_group_path;
  try {
    entry_group_path = avahi_->EntryGroupNew();
  } catch (const sdbus::Error &e) {
    DBUS_LOG_METHOD_CALL_ERROR(avahi_, "EntryGroupNew", e);
    return false;
  }

  auto entry_group =
      std::make_unique<avahi::EntryGroup>(*system_bus_, entry_group_path);
  LOG(INFO) << __func__ << ": Adding avahi service with service type: " << nsd_service_info.GetServiceType();

  try {
    entry_group->AddService(
        -1,  // AVAHI_IF_UNSPEC
        -1,  // AVAHI_PROTO_UNSPED
        0, nsd_service_info.GetServiceName(), nsd_service_info.GetServiceType(),
        std::string(), std::string(), nsd_service_info.GetPort(), txt_records);
    entry_group->Commit();
  } catch (const sdbus::Error &e) {
    LOG(ERROR) << __func__ << ": Got error '" << e.getName()
                       << "' with message '" << e.getMessage()
                       << "' while adding service";
    return false;
  }

  absl::MutexLock l(&entry_groups_mutex_);
  entry_groups_.insert({*key, std::move(entry_group)});

  return true;
}

bool WifiLanMedium::StopAdvertising(const NsdServiceInfo &nsd_service_info) {
  auto key = entry_group_key(nsd_service_info);
  if (!key.has_value()) {
    return false;
  }

  absl::MutexLock l(&entry_groups_mutex_);
  if (entry_groups_.count(*key) == 0) {
    LOG(ERROR) << __func__
                       << ": Advertising is already inactive for this service.";
    return false;
  }

  entry_groups_.erase(*key);
  return true;
}

bool WifiLanMedium::StartDiscovery(
    const std::string &service_type,
    api::WifiLanMedium::DiscoveredServiceCallback callback) {
  {
    absl::ReaderMutexLock l(&service_browsers_mutex_);
    if (service_browsers_.count(service_type) != 0) {
      auto &object = service_browsers_[service_type];
      LOG(ERROR) << __func__ << ": A service browser for service type "
                         << service_type << " already exists at "
                         << object->getObjectPath();
      return false;
    }
  }

  avahi_->SetDiscoveryCallback(std::move(callback));

  try {
    sdbus::ObjectPath browser_object_path =
        avahi_->ServiceBrowserPrepare(-1,  // AVAHI_IF_UNSPEC
                                      -1,  // AVAHI_PROTO_UNSPED
                                      service_type, std::string(), 0);
    LOG(INFO)
        << __func__
        << ": Created a new org.freedesktop.Avahi.ServiceBrowser object at "
        << browser_object_path << " for service_type: " << service_type;

    absl::MutexLock l(&service_browsers_mutex_);
    service_browsers_.emplace(
        service_type,
        std::make_unique<avahi::ServiceBrowser>(
            *system_bus_, browser_object_path, avahi_));
  } catch (const sdbus::Error &e) {
    DBUS_LOG_METHOD_CALL_ERROR(avahi_, "ServiceBrowserPrepare", e);
    return false;
  }

  service_browsers_mutex_.ReaderLock();
  auto &browser = service_browsers_[service_type];
  service_browsers_mutex_.ReaderUnlock();

  try {
    LOG(INFO) << __func__ << ": Starting service discovery for "
                         << browser->getObjectPath();
    browser->Start();
  } catch (const sdbus::Error &e) {
    DBUS_LOG_METHOD_CALL_ERROR(browser, "Start", e);
    return false;
  }

  return true;
}

bool WifiLanMedium::StopDiscovery(const std::string &service_type) {
  absl::MutexLock l(&service_browsers_mutex_);

  if (service_browsers_.count(service_type) == 0) {
    LOG(ERROR) << __func__ << ": Service type " << service_type
                       << " has not been registered for discovery";
    return false;
  }
  service_browsers_.erase(service_type);

  return true;
}

std::unique_ptr<api::WifiLanSocket> WifiLanMedium::ConnectToService(
    const std::string &ip_address, int port,
    CancellationFlag *cancellation_flag) {
  auto socket = TCPSocket::Connect(ip_address, port);
  if (!socket.has_value()) return nullptr;
  return std::make_unique<WifiLanSocket>(*socket);
}

std::unique_ptr<api::WifiLanServerSocket> WifiLanMedium::ListenForService(
    int port) {
  LOG(INFO)<< __func__ << ": Listening for service WifiLanMedium";
  auto socket = TCPServerSocket::Listen(std::nullopt, port);
  if (!socket.has_value()) return nullptr;

  return std::make_unique<WifiLanServerSocket>(std::move(*socket),
                                               network_manager_);
}

namespace {

bool IsIPv4LinkLocal(const std::string& ip_address) {
  struct in_addr addr;
  if (inet_pton(AF_INET, ip_address.c_str(), &addr) != 1) {
    return false;
  }
  // Link-local range: 169.254.0.0/16
  uint32_t ip = ntohl(addr.s_addr);
  return (ip & 0xFFFF0000) == 0xA9FE0000;
}

}  // namespace

api::UpgradeAddressInfo WifiLanMedium::GetUpgradeAddressCandidates(
    const api::WifiLanServerSocket& server_socket) {
  api::UpgradeAddressInfo result;
  uint16_t port = server_socket.GetPort();
  std::vector<ServiceAddress> ipv4_addresses;

  std::vector<sdbus::ObjectPath> connection_paths;
  try {
    connection_paths = network_manager_->ActiveConnections();
  } catch (const sdbus::Error& e) {
    DBUS_LOG_PROPERTY_GET_ERROR(network_manager_, "ActiveConnections", e);
    return result;
  }

  for (auto& path : connection_paths) {
    auto active_connection =
        std::make_unique<networkmanager::ActiveConnection>(system_bus_, path);
    std::string conn_type;
    try {
      conn_type = active_connection->Type();
    } catch (const sdbus::Error& e) {
      DBUS_LOG_PROPERTY_GET_ERROR(active_connection, "Type", e);
      continue;
    }

    // Only use WiFi and Ethernet interfaces for upgrade
    if (conn_type != "802-11-wireless" && conn_type != "802-3-ethernet") {
      continue;
    }

    bool has_ipv4_address = false;
    // TODO: Add IPv6 support when IP6Config interface is available

    // Get IPv4 addresses
    auto ip4addresses = active_connection->GetIP4Addresses();
    for (const auto& ip_str : ip4addresses) {
      // Filter out link-local addresses
      if (IsIPv4LinkLocal(ip_str)) {
        continue;
      }

      struct in_addr addr;
      if (inet_pton(AF_INET, ip_str.c_str(), &addr) != 1) {
        LOG(ERROR) << __func__ << ": Invalid IPv4 address: " << ip_str;
        continue;
      }

      // Convert to vector of chars in network byte order
      std::vector<char> addr_bytes(4);
      std::memcpy(addr_bytes.data(), &addr.s_addr, 4);
      
      ipv4_addresses.push_back(
          ServiceAddress{.address = std::move(addr_bytes), .port = port});
      has_ipv4_address = true;
    }

    if (has_ipv4_address) {
      result.num_interfaces++;
    }
  }

  // Append IPv4 addresses to the result
  result.address_candidates.insert(result.address_candidates.end(),
                                   ipv4_addresses.begin(),
                                   ipv4_addresses.end());

  return result;
};
}  // namespace linux
}  // namespace nearby
