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
#include <cstdint>
#include <cstring>
#include <memory>
#include <random>

#include "internal/platform/implementation/linux/dbus.h"
#include "internal/platform/implementation/linux/linux_flags.h"
#include "internal/platform/implementation/linux/network_manager.h"
#include "internal/platform/implementation/linux/network_manager_access_point.h"
#include "internal/platform/implementation/linux/utils.h"
#include "internal/platform/implementation/linux/wifi_hotspot.h"
#include "internal/platform/implementation/linux/wifi_hotspot_server_socket.h"
#include "internal/platform/implementation/linux/wifi_hotspot_socket.h"
#include "internal/platform/implementation/linux/wifi_medium.h"
#include "internal/platform/implementation/wifi.h"
#include "internal/platform/logging.h"

namespace nearby {
namespace linux {
namespace {

// Prefer non-DFS channels that are broadly available in the 00 (world) regulatory
// domain for AP mode.
constexpr int kPreferred24GhzChannel = 6;
constexpr int kPreferred5GhzChannel = 36;
// NM_SETTING_WIRELESS_CHANNEL_WIDTH_* (nm-settings: 40mhz=40, 80mhz=80).
constexpr int32_t k40MhzChannelWidth = 40;
constexpr int32_t k80MhzChannelWidth = 80;

}  // namespace

std::unique_ptr<api::WifiHotspotSocket>
NetworkManagerWifiHotspotMedium::ConnectToService(
    absl::string_view ip_address, int port,
    CancellationFlag *cancellation_flag) {
  if (!ConnectedToWifi()) {
    LOG(ERROR)
        << __func__
        << ": Cannot connect to service without an active WiFi hotspot";
    return nullptr;
  }

  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    LOG(ERROR) << __func__
                       << ": Error opening socket: " << std::strerror(errno);
    return nullptr;
  }

  LOG(INFO) << __func__ << ": Connecting to " << ip_address << ":"
                       << port;
  struct sockaddr_in addr {};
  addr.sin_addr.s_addr = inet_addr(std::string(ip_address).c_str());
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);

  auto ret =
      connect(sock, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr));
  if (ret < 0) {
    LOG(ERROR) << __func__ << ": Error connecting to socket: "
                       << std::strerror(errno);
    return nullptr;
  }

  return std::make_unique<WifiHotspotSocket>(sock);
}

std::unique_ptr<api::WifiHotspotServerSocket>
NetworkManagerWifiHotspotMedium::ListenForService(int port) {
  if (!WifiHotspotActive()) {
    LOG(ERROR)
        << __func__
        << ": Cannot connect to service without an active WiFi hotspot";
    return nullptr;
  }

  auto active_connection = wireless_device_->GetActiveConnection();
  if (active_connection == nullptr) {
    return nullptr;
  }

  auto ip4addresses = active_connection->GetIP4Addresses();
  if (ip4addresses.empty()) {
    LOG(ERROR)
        << __func__
        << "Could not find any IPv4 addresses for active connection "
        << active_connection->getProxy().getObjectPath();
    return nullptr;
  }

  auto sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    LOG(ERROR) << __func__
                       << ": Error opening socket: " << std::strerror(errno);
    return nullptr;
  }

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr(ip4addresses[0].c_str());
  addr.sin_port = htons(port);

  auto ret =
      bind(sock, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr));
  if (ret < 0) {
    LOG(ERROR) << __func__
                       << ": Error binding to socket: " << std::strerror(errno);
    return nullptr;
  }

  LOG(INFO) << __func__ << ": Listening for services on "
                       << ip4addresses[0] << ":" << port << " on device "
                       << wireless_device_->getProxy().getObjectPath();

  ret = listen(sock, 0);
  if (ret < 0) {
    LOG(ERROR) << __func__ << ": Error listening on socket: "
                       << std::strerror(errno);
    return nullptr;
  }

  return std::make_unique<NetworkManagerWifiHotspotServerSocket>(
      sock, std::move(active_connection), network_manager_);
}

bool NetworkManagerWifiHotspotMedium::StartWifiHotspot(
    HotspotCredentials *hotspot_credentials) {
  if (WifiHotspotActive()) {
    LOG(ERROR) << __func__ << ": " << wireless_device_->getProxy().getObjectPath()
                       << ": cannot start WiFi hotspot, a hotspot is already "
                          "active on this device";
    return false;
  }

  std::string ssid = RandSSID();
  hotspot_credentials->SetSSID(ssid);

  std::string password = RandWPAPassphrase();
  hotspot_credentials->SetPassword(password);

  auto connection_id = NewUuidStr();
  if (!connection_id.has_value()) {
    LOG(ERROR) << __func__ << ": could not generate a connection UUID";
    return false;
  }

  // Get device capabilities to select the best available band
  api::WifiCapability& capability = wireless_device_->GetCapability();
  std::string selected_band;
  int selected_channel = kPreferred24GhzChannel;
  const bool enable_5ghz_hotspot = Is5GhzHotspotEnabled();
  if (enable_5ghz_hotspot &&
      (capability.supports_6_ghz || capability.supports_5_ghz)) {
    selected_band = "a";  // 5/6 GHz - NetworkManager uses "a" for both 5 GHz and 6 GHz
    selected_channel = kPreferred5GhzChannel;
    LOG(INFO) << __func__
              << ": Device supports 5/6 GHz, using 5/6 GHz band on channel "
              << selected_channel;
  } else {
    selected_band = "bg";  // 2.4 GHz
    selected_channel = kPreferred24GhzChannel;
    if (!enable_5ghz_hotspot &&
        (capability.supports_6_ghz || capability.supports_5_ghz)) {
      LOG(INFO) << __func__
                << ": 5/6 GHz hotspot is disabled by feature flag, using 2.4 "
                << "GHz band on channel " << selected_channel;
    } else {
      LOG(INFO) << __func__
                << ": Device supports only 2.4 GHz, using 2.4 GHz band on "
                << "channel " << selected_channel;
    }
  }

  const int32_t fallback_channel_width =
      selected_band == "a" ? k80MhzChannelWidth : k40MhzChannelWidth;

  std::unique_ptr<networkmanager::ActiveConnection> active_conn;
  for (bool include_channel_width : {true, false}) {
    std::map<std::string, std::map<std::string, sdbus::Variant>>
        connection_settings{
            {
                "connection",
                {{"uuid", sdbus::Variant(*connection_id)},
                 {"id", sdbus::Variant("Google Nearby Hotspot")},
                 {"type", sdbus::Variant("802-11-wireless")},
                 {"zone", sdbus::Variant("Public")}},
            },
            {"802-11-wireless",
             {{"assigned-mac-address", sdbus::Variant("random")},
              {"ap-isolation", sdbus::Variant(networkmanager::constants::kNMTernaryFalse)},
              {"mode", sdbus::Variant("ap")},
              {"band", sdbus::Variant(selected_band)},
              {"channel", sdbus::Variant(selected_channel)},
              {"ssid", sdbus::Variant(std::vector<uint8_t>(ssid.begin(), ssid.end()))},
              {"security", sdbus::Variant("802-11-wireless-security")}}},
            {"802-11-wireless-security",
             {{"pmf", sdbus::Variant(networkmanager::constants::setting::kWirelessSecurityPMFDisable)},
              {"key-mgmt", sdbus::Variant("wpa-psk")},
              {"psk", sdbus::Variant(password)}}},
            {"ipv4", {{"method", sdbus::Variant("shared")}}},
            {"ipv6",
             {
                 {"addr-gen-mode",
                  sdbus::Variant(networkmanager::constants::setting::kIP6ConfigAddrGenModeStablePrivacy)},
                 {"method", sdbus::Variant("shared")},
             }}};
    if (include_channel_width) {
      connection_settings["802-11-wireless"]["channel-width"] =
          sdbus::Variant(fallback_channel_width);
    }
    try {
      auto [path, active_path, result] =
          network_manager_->AddAndActivateConnection2(
              connection_settings,
              wireless_device_->getProxy().getObjectPath(),
              sdbus::ObjectPath("/"),
              {{"persist", sdbus::Variant("volatile")},
               {"bind-activation", sdbus::Variant("dbus-client")}});
      active_conn = std::make_unique<networkmanager::ActiveConnection>(
          system_bus_, active_path);
      break;
    } catch (const sdbus::Error &e) {
      if (include_channel_width) {
        LOG(WARNING) << __func__
                     << ": Failed to set channel-width="
                     << fallback_channel_width << "mhz: "
                     << e.getName() << ": " << e.getMessage()
                     << ". Retrying without channel-width.";
        continue;
      }
      DBUS_LOG_METHOD_CALL_ERROR(network_manager_, "AddAndActivateConnection2",
                                 e);
      return false;
    }
  }

  if (active_conn == nullptr) {
    return false;
  }

  auto [reason, timeout] = active_conn->WaitForConnection(absl::Seconds(60));
  if (timeout) {
    LOG(ERROR)
        << __func__ << ": "
        << ": timed out while waiting for connection "
        << active_conn->getProxy().getObjectPath()
        << " to be activated, last NMActiveConnectionStateReason: "
        << reason->ToString();
    DisconnectWifiHotspot();
    return false;
  }

  // Get the frequency of the active hotspot
  try {
    sdbus::ObjectPath active_ap_path = wireless_device_->ActiveAccessPoint();
    if (!active_ap_path.empty()) {
      auto access_point = NetworkManagerAccessPoint(*system_bus_, active_ap_path);
      uint32_t frequency = access_point.Frequency();
      hotspot_credentials->SetFrequency(static_cast<int>(frequency));
      LOG(INFO) << __func__ << ": Hotspot frequency set to " << frequency
                << " MHz";
    } else {
      LOG(WARNING) << __func__
                   << ": Could not get active access point to retrieve frequency";
    }
  } catch (const sdbus::Error &e) {
    LOG(WARNING) << __func__ << ": Could not retrieve hotspot frequency: "
                 << e.what();
  }

  LOG(INFO) << __func__ << ": Started a WiFi hotspot on device "
                    << wireless_device_->getProxy().getObjectPath() << " at "
                    << active_conn->getProxy().getObjectPath();
  return true;
}

bool NetworkManagerWifiHotspotMedium::StopWifiHotspot() {
  if (!WifiHotspotActive()) {
    LOG(ERROR)
        << __func__ << ": " << wireless_device_->getProxy().getObjectPath()
        << ": Cannot stop WiFi hotspot as a WiFi hotspot is not active";
  }

  // Get the active connection object for the hotspot AP.
  sdbus::ObjectPath active_ap_path;

  try {
    active_ap_path = wireless_device_->ActiveAccessPoint();
    if (active_ap_path.empty()) {
      LOG(ERROR) << __func__ << ": No active access points on "
                         << wireless_device_->getProxy().getObjectPath();
      return false;
    }
  } catch (const sdbus::Error &e) {
    DBUS_LOG_PROPERTY_GET_ERROR(wireless_device_, "ActiveAccessPoint", e);
  }

  auto object_manager = networkmanager::ObjectManager(system_bus_);
  auto active_connection = wireless_device_->GetActiveConnection();
  if (active_connection == nullptr) {
    LOG(ERROR)
        << __func__
        << ": Could not find an active connection using the access point "
        << active_ap_path;
    return false;
  }

  LOG(INFO) << __func__ << ": " << wireless_device_->getProxy().getObjectPath()
                    << ": Deactivating active connection "
                    << active_connection->getProxy().getObjectPath();

  try {
    network_manager_->DeactivateConnection(active_connection->getProxy().getObjectPath());
  } catch (const sdbus::Error &e) {
    DBUS_LOG_METHOD_CALL_ERROR(network_manager_, "DeactivateConnection", e);
    return false;
  }

  return true;
}

bool NetworkManagerWifiHotspotMedium::ConnectWifiHotspot(
    const HotspotCredentials& hotspot_credentials) {

  auto ssid = hotspot_credentials.GetSSID();
  auto password = hotspot_credentials.GetPassword();

  return wireless_device_->ConnectToNetwork(ssid, password,
                                            NetworkManagerWifiMedium::WifiAuthType::kWpaPsk) ==
         NetworkManagerWifiMedium::WifiConnectionStatus::kConnected;
}

bool NetworkManagerWifiHotspotMedium::DisconnectWifiHotspot() {
  if (!ConnectedToWifi()) {
    LOG(ERROR) << __func__ << ": Not connected to a WiFi hotspot";
    return false;
  }

  auto active_connection = wireless_device_->GetActiveConnection();
  if (active_connection == nullptr) {
    return false;
  }

  try {
    network_manager_->DeactivateConnection(active_connection->getProxy().getObjectPath());
  } catch (const sdbus::Error &e) {
    DBUS_LOG_METHOD_CALL_ERROR(network_manager_, "DeactivateConnection", e);
    return false;
  }

  return true;
}

bool NetworkManagerWifiHotspotMedium::WifiHotspotActive() {
  try {
    auto mode = wireless_device_->Mode();
    return mode == networkmanager::constants::kNM80211ModeAP;
  } catch (const sdbus::Error &e) {
    DBUS_LOG_PROPERTY_GET_ERROR(wireless_device_, "Mode", e);
    return false;
  }
}

bool NetworkManagerWifiHotspotMedium::ConnectedToWifi() {
  try {
    auto mode = wireless_device_->Mode();
    return mode == networkmanager::constants::kNM80211ModeInfra;
  } catch (const sdbus::Error &e) {
    DBUS_LOG_PROPERTY_GET_ERROR(wireless_device_, "Mode", e);
    return false;
  }
}

}  // namespace linux
}  // namespace nearby
