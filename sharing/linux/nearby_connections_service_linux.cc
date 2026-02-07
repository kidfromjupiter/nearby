// Copyright 2025 Google LLC
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

#include "sharing/linux/nearby_connections_service_linux.h"

#include <stdint.h>

#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "absl/strings/string_view.h"
#include "absl/time/time.h"
#include "absl/types/span.h"
#include "connections/advertising_options.h"
#include "connections/connection_options.h"
#include "connections/discovery_options.h"
#include "connections/listeners.h"
#include "connections/payload.h"
#include "connections/payload_type.h"
#include "connections/status.h"
#include "connections/strategy.h"
#include "internal/base/file_path.h"
#include "internal/platform/byte_array.h"
#include "internal/platform/file.h"
#include "internal/platform/logging.h"
#include "internal/platform/mac_address.h"

namespace nearby {
namespace sharing::linux {
namespace {

using ::nearby::connections::ConnectionRequestInfo;
using ::nearby::connections::ConnectionResponseInfo;
using ::nearby::connections::PayloadProgressInfo;
using ::nearby::connections::PayloadType;

using NcAdvertisingOptions = ::nearby::connections::AdvertisingOptions;
using NcConnectionOptions = ::nearby::connections::ConnectionOptions;
using NcDiscoveryListener = ::nearby::connections::DiscoveryListener;
using NcDiscoveryOptions = ::nearby::connections::DiscoveryOptions;
using NcDistanceInfo = ::nearby::connections::DistanceInfo;
using NcMedium = ::nearby::connections::Medium;
using NcPayload = ::nearby::connections::Payload;
using NcPayloadListener = ::nearby::connections::PayloadListener;
using NcResultCallback = ::nearby::connections::ResultCallback;
using NcStatus = ::nearby::connections::Status;
using NcStrategy = ::nearby::connections::Strategy;

Status ConvertStatus(NcStatus status) {
  return static_cast<Status>(status.value);
}

NcResultCallback MakeResultCallback(std::function<void(Status status)> callback) {
  return [callback = std::move(callback)](NcStatus status) {
    callback(ConvertStatus(status));
  };
}

Payload ConvertPayload(NcPayload payload) {
  switch (payload.GetType()) {
    case PayloadType::kBytes: {
      const ByteArray& bytes = payload.AsBytes();
      std::string data = std::string(bytes);
      return Payload(payload.GetId(),
                     std::vector<uint8_t>(data.begin(), data.end()));
    }
    case PayloadType::kFile: {
      std::string file_path = payload.AsFile()->GetFilePath();
      std::string parent_folder = payload.GetParentFolder();
      VLOG(1) << __func__ << ": Payload file_path=" << file_path
              << ", parent_folder=" << parent_folder;
      return Payload(payload.GetId(), FilePath(file_path), parent_folder);
    }
    default:
      return Payload();
  }
}

NcPayload ConvertToNcPayload(Payload payload) {
  switch (payload.content.type) {
    case PayloadContent::Type::kFile: {
      std::string file_path = payload.content.file_payload.file_path.ToString();
      std::string file_name =
          payload.content.file_payload.file_path.GetFileName().ToString();
      std::string parent_folder = payload.content.file_payload.parent_folder;
      std::replace(parent_folder.begin(), parent_folder.end(), '\\', '/');
      VLOG(1) << __func__ << ": NC Payload file_path=" << file_path
              << ", parent_folder=" << parent_folder;
      nearby::InputFile input_file(file_path);
      return NcPayload(payload.id, parent_folder, file_name,
                       std::move(input_file));
    }
    case PayloadContent::Type::kBytes: {
      std::vector<uint8_t> bytes = payload.content.bytes_payload.bytes;
      return NcPayload(payload.id,
                       ByteArray(std::string(bytes.begin(), bytes.end())));
    }
    default:
      return NcPayload();
  }
}

NcStrategy ConvertStrategy(Strategy strategy) {
  switch (strategy) {
    case Strategy::kP2pCluster:
      return NcStrategy::kP2pCluster;
    case Strategy::kP2pPointToPoint:
      return NcStrategy::kP2pPointToPoint;
    case Strategy::kP2pStar:
      return NcStrategy::kP2pStar;
  }
  return NcStrategy::kP2pPointToPoint;
}

ConnectionInfo ConvertConnectionInfo(const ConnectionResponseInfo& info) {
  ConnectionInfo connection_info;
  connection_info.authentication_token = info.authentication_token;
  std::string remote_endpoint_info = std::string(info.remote_endpoint_info);
  connection_info.endpoint_info = std::vector<uint8_t>(
      remote_endpoint_info.begin(), remote_endpoint_info.end());
  connection_info.is_incoming_connection = info.is_incoming_connection;
  std::string raw_authentication_token =
      std::string(info.raw_authentication_token);
  connection_info.raw_authentication_token = std::vector<uint8_t>(
      raw_authentication_token.begin(), raw_authentication_token.end());
  return connection_info;
}

}  // namespace

NearbyConnectionsServiceLinux::NearbyConnectionsServiceLinux()
    : core_(&router_) {}

NearbyConnectionsServiceLinux::NearbyConnectionsServiceLinux(
    nearby::analytics::EventLogger* event_logger)
    : core_(event_logger, &router_) {}

NearbyConnectionsServiceLinux::~NearbyConnectionsServiceLinux() = default;

void NearbyConnectionsServiceLinux::StartAdvertising(
    absl::string_view service_id, const std::vector<uint8_t>& endpoint_info,
    const AdvertisingOptions& advertising_options,
    ConnectionListener advertising_listener,
    std::function<void(Status status)> callback) {
  advertising_listener_ = std::move(advertising_listener);

  NcAdvertisingOptions options{};
  options.strategy = ConvertStrategy(advertising_options.strategy);
  options.allowed.ble = advertising_options.allowed_mediums.ble;
  options.allowed.bluetooth = advertising_options.allowed_mediums.bluetooth;
  options.allowed.web_rtc = advertising_options.allowed_mediums.web_rtc;
  options.allowed.wifi_lan = advertising_options.allowed_mediums.wifi_lan;
  options.allowed.wifi_hotspot = advertising_options.allowed_mediums.wifi_hotspot;
  options.auto_upgrade_bandwidth = advertising_options.auto_upgrade_bandwidth;
  options.enforce_topology_constraints =
      advertising_options.enforce_topology_constraints;
  options.enable_bluetooth_listening =
      advertising_options.enable_bluetooth_listening;
  options.enable_webrtc_listening = advertising_options.enable_webrtc_listening;
  options.use_stable_endpoint_id = advertising_options.use_stable_endpoint_id;
  options.force_new_endpoint_id = advertising_options.force_new_endpoint_id;
  options.fast_advertisement_service_uuid =
      advertising_options.fast_advertisement_service_uuid.uuid;

  ConnectionRequestInfo request_info;
  request_info.endpoint_info =
      ByteArray(std::string(endpoint_info.begin(), endpoint_info.end()));
  request_info.listener.initiated_cb = [this](const std::string& endpoint_id,
                                              const ConnectionResponseInfo& info) {
    advertising_listener_.initiated_cb(endpoint_id, ConvertConnectionInfo(info));
  };
  request_info.listener.accepted_cb = [this](const std::string& endpoint_id) {
    advertising_listener_.accepted_cb(endpoint_id);
  };
  request_info.listener.rejected_cb =
      [this](const std::string& endpoint_id, NcStatus status) {
        advertising_listener_.rejected_cb(endpoint_id, ConvertStatus(status));
      };
  request_info.listener.disconnected_cb =
      [this](const std::string& endpoint_id) {
        payload_listeners_.erase(endpoint_id);
        advertising_listener_.disconnected_cb(endpoint_id);
      };
  request_info.listener.bandwidth_changed_cb =
      [this](const std::string& endpoint_id, NcMedium medium) {
        advertising_listener_.bandwidth_changed_cb(endpoint_id,
                                                   static_cast<Medium>(medium));
      };

  core_.StartAdvertising(service_id, options, std::move(request_info),
                         MakeResultCallback(std::move(callback)));
}

void NearbyConnectionsServiceLinux::StopAdvertising(
    absl::string_view service_id, std::function<void(Status status)> callback) {
  static_cast<void>(service_id);
  core_.StopAdvertising(MakeResultCallback(std::move(callback)));
}

void NearbyConnectionsServiceLinux::StartDiscovery(
    absl::string_view service_id, const DiscoveryOptions& discovery_options,
    DiscoveryListener discovery_listener,
    std::function<void(Status status)> callback) {
  discovery_listener_ = std::move(discovery_listener);

  NcDiscoveryOptions options{};
  options.strategy = ConvertStrategy(discovery_options.strategy);
  options.allowed.SetAll(false);
  options.allowed.ble = discovery_options.allowed_mediums.ble;
  options.allowed.bluetooth = discovery_options.allowed_mediums.bluetooth;
  options.allowed.web_rtc = discovery_options.allowed_mediums.web_rtc;
  options.allowed.wifi_lan = discovery_options.allowed_mediums.wifi_lan;
  options.allowed.wifi_hotspot = discovery_options.allowed_mediums.wifi_hotspot;
  if (discovery_options.fast_advertisement_service_uuid.has_value()) {
    options.fast_advertisement_service_uuid =
        (*discovery_options.fast_advertisement_service_uuid).uuid;
  }
  options.is_out_of_band_connection =
      discovery_options.is_out_of_band_connection;
  if (discovery_options.alternate_service_uuid.has_value()) {
    options.ble_options.alternate_uuid =
        discovery_options.alternate_service_uuid;
  }

  NcDiscoveryListener listener;
  listener.endpoint_found_cb = [this](const std::string& endpoint_id,
                                      const ByteArray& endpoint_info,
                                      const std::string& discovered_service_id) {
    std::string endpoint_info_data = std::string(endpoint_info);
    discovery_listener_.endpoint_found_cb(
        endpoint_id,
        DiscoveredEndpointInfo(std::vector<uint8_t>(endpoint_info_data.begin(),
                                                    endpoint_info_data.end()),
                               discovered_service_id));
  };
  listener.endpoint_lost_cb = [this](const std::string& endpoint_id) {
    discovery_listener_.endpoint_lost_cb(endpoint_id);
  };
  listener.endpoint_distance_changed_cb = [this](const std::string& endpoint_id,
                                                 NcDistanceInfo distance_info) {
    discovery_listener_.endpoint_distance_changed_cb(
        endpoint_id, static_cast<DistanceInfo>(distance_info));
  };

  core_.StartDiscovery(service_id, options, std::move(listener),
                       MakeResultCallback(std::move(callback)));
}

void NearbyConnectionsServiceLinux::StopDiscovery(
    absl::string_view service_id, std::function<void(Status status)> callback) {
  static_cast<void>(service_id);
  core_.StopDiscovery(MakeResultCallback(std::move(callback)));
}

void NearbyConnectionsServiceLinux::RequestConnection(
    absl::string_view service_id, const std::vector<uint8_t>& endpoint_info,
    absl::string_view endpoint_id, const ConnectionOptions& connection_options,
    ConnectionListener connection_listener,
    std::function<void(Status status)> callback) {
  static_cast<void>(service_id);
  connection_listener_ = std::move(connection_listener);

  NcConnectionOptions options{};
  options.allowed.ble = connection_options.allowed_mediums.ble;
  options.allowed.bluetooth = connection_options.allowed_mediums.bluetooth;
  options.allowed.web_rtc = connection_options.allowed_mediums.web_rtc;
  options.allowed.wifi_lan = connection_options.allowed_mediums.wifi_lan;
  options.allowed.wifi_hotspot = connection_options.allowed_mediums.wifi_hotspot;
  if (connection_options.keep_alive_interval.has_value()) {
    options.keep_alive_interval_millis =
        *connection_options.keep_alive_interval / absl::Milliseconds(1);
  }
  if (connection_options.keep_alive_timeout.has_value()) {
    options.keep_alive_timeout_millis =
        *connection_options.keep_alive_timeout / absl::Milliseconds(1);
  }
  if (connection_options.remote_bluetooth_mac_address.has_value()) {
    MacAddress mac_address;
    MacAddress::FromBytes(
        absl::MakeConstSpan(*connection_options.remote_bluetooth_mac_address),
        mac_address);
    options.remote_bluetooth_mac_address = mac_address;
  }
  options.non_disruptive_hotspot_mode =
      connection_options.non_disruptive_hotspot_mode;

  ConnectionRequestInfo request_info;
  request_info.endpoint_info =
      ByteArray(std::string(endpoint_info.begin(), endpoint_info.end()));
  request_info.listener.initiated_cb = [this](const std::string& endpoint_id,
                                              const ConnectionResponseInfo& info) {
    connection_listener_.initiated_cb(endpoint_id, ConvertConnectionInfo(info));
  };
  request_info.listener.accepted_cb = [this](const std::string& endpoint_id) {
    connection_listener_.accepted_cb(endpoint_id);
  };
  request_info.listener.rejected_cb =
      [this](const std::string& endpoint_id, NcStatus status) {
        connection_listener_.rejected_cb(endpoint_id, ConvertStatus(status));
      };
  request_info.listener.disconnected_cb =
      [this](const std::string& endpoint_id) {
        payload_listeners_.erase(endpoint_id);
        connection_listener_.disconnected_cb(endpoint_id);
      };
  request_info.listener.bandwidth_changed_cb =
      [this](const std::string& endpoint_id, NcMedium medium) {
        connection_listener_.bandwidth_changed_cb(endpoint_id,
                                                  static_cast<Medium>(medium));
      };

  core_.RequestConnection(endpoint_id, std::move(request_info), options,
                          MakeResultCallback(std::move(callback)));
}

void NearbyConnectionsServiceLinux::DisconnectFromEndpoint(
    absl::string_view service_id, absl::string_view endpoint_id,
    std::function<void(Status status)> callback) {
  static_cast<void>(service_id);
  payload_listeners_.erase(std::string(endpoint_id));
  core_.DisconnectFromEndpoint(endpoint_id,
                               MakeResultCallback(std::move(callback)));
}

void NearbyConnectionsServiceLinux::SendPayload(
    absl::string_view service_id, absl::Span<const std::string> endpoint_ids,
    std::unique_ptr<Payload> payload,
    std::function<void(Status status)> callback) {
  static_cast<void>(service_id);
  core_.SendPayload(endpoint_ids, ConvertToNcPayload(*payload),
                    MakeResultCallback(std::move(callback)));
}

void NearbyConnectionsServiceLinux::CancelPayload(
    absl::string_view service_id, int64_t payload_id,
    std::function<void(Status status)> callback) {
  static_cast<void>(service_id);
  core_.CancelPayload(payload_id, MakeResultCallback(std::move(callback)));
}

void NearbyConnectionsServiceLinux::InitiateBandwidthUpgrade(
    absl::string_view service_id, absl::string_view endpoint_id,
    std::function<void(Status status)> callback) {
  static_cast<void>(service_id);
  core_.InitiateBandwidthUpgrade(endpoint_id,
                                 MakeResultCallback(std::move(callback)));
}

void NearbyConnectionsServiceLinux::AcceptConnection(
    absl::string_view service_id, absl::string_view endpoint_id,
    PayloadListener payload_listener,
    std::function<void(Status status)> callback) {
  static_cast<void>(service_id);
  payload_listeners_[std::string(endpoint_id)] = std::move(payload_listener);

  NcPayloadListener service_payload_listener{
      .payload_cb =
          [this](absl::string_view endpoint_id, NcPayload payload) {
            auto it = payload_listeners_.find(std::string(endpoint_id));
            if (it == payload_listeners_.end()) {
              return;
            }
            VLOG(1) << "payload callback id=" << payload.GetId();

            switch (payload.GetType()) {
              case PayloadType::kBytes:
              case PayloadType::kFile:
                it->second.payload_cb(endpoint_id,
                                      ConvertPayload(std::move(payload)));
                break;
              default:
                break;
            }
          },
      .payload_progress_cb =
          [this](absl::string_view endpoint_id, const PayloadProgressInfo& info) {
            auto it = payload_listeners_.find(std::string(endpoint_id));
            if (it == payload_listeners_.end()) {
              return;
            }

            PayloadTransferUpdate transfer_update;
            transfer_update.bytes_transferred = info.bytes_transferred;
            transfer_update.payload_id = info.payload_id;
            transfer_update.status = static_cast<PayloadStatus>(info.status);
            transfer_update.total_bytes = info.total_bytes;
            VLOG(1) << "payload transfer update id=" << info.payload_id;
            it->second.payload_progress_cb(endpoint_id, transfer_update);
          }};

  core_.AcceptConnection(endpoint_id, std::move(service_payload_listener),
                         MakeResultCallback(std::move(callback)));
}

void NearbyConnectionsServiceLinux::StopAllEndpoints(
    std::function<void(Status status)> callback) {
  payload_listeners_.clear();
  core_.StopAllEndpoints(MakeResultCallback(std::move(callback)));
}

void NearbyConnectionsServiceLinux::SetCustomSavePath(
    absl::string_view path, std::function<void(Status status)> callback) {
  core_.SetCustomSavePath(path, MakeResultCallback(std::move(callback)));
}

std::string NearbyConnectionsServiceLinux::Dump() const {
  // NearbyConnectionsService requires const Dump(), but Core::Dump() is
  // non-const in this codebase.
  return const_cast<connections::Core&>(core_).Dump();
}

}  // namespace sharing::linux
}  // namespace nearby
