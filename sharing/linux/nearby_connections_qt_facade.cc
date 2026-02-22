#include "sharing/linux/nearby_connections_qt_facade.h"

#include <atomic>
#include <utility>

#include "connections/implementation/flags/nearby_connections_feature_flags.h"
#include "internal/base/file_path.h"
#include "internal/flags/nearby_flags.h"
#include "sharing/linux/nearby_connections_service_linux.h"
#include "sharing/nearby_connections_types.h"

namespace nearby::sharing {

namespace {

using Facade = NearbyConnectionsQtFacade;
using NativeService = nearby::sharing::NearbyConnectionsService;

std::atomic<int64_t> g_next_payload_id{1};

int64_t NextPayloadId() { return g_next_payload_id.fetch_add(1); }

std::string DecodePeerName(const std::vector<uint8_t>& endpoint_info) {
  if (endpoint_info.empty()) {
    return {};
  }
  std::string decoded(endpoint_info.begin(), endpoint_info.end());
  while (!decoded.empty() && decoded.back() == '\0') {
    decoded.pop_back();
  }
  return decoded;
}

Facade::Status ToFacadeStatus(nearby::sharing::Status status) {
  switch (status) {
    case nearby::sharing::Status::kSuccess:
      return Facade::Status::kSuccess;
    case nearby::sharing::Status::kError:
      return Facade::Status::kError;
    case nearby::sharing::Status::kOutOfOrderApiCall:
      return Facade::Status::kOutOfOrderApiCall;
    case nearby::sharing::Status::kAlreadyHaveActiveStrategy:
      return Facade::Status::kAlreadyHaveActiveStrategy;
    case nearby::sharing::Status::kAlreadyAdvertising:
      return Facade::Status::kAlreadyAdvertising;
    case nearby::sharing::Status::kAlreadyDiscovering:
      return Facade::Status::kAlreadyDiscovering;
    case nearby::sharing::Status::kAlreadyListening:
      return Facade::Status::kAlreadyListening;
    case nearby::sharing::Status::kEndpointIOError:
      return Facade::Status::kEndpointIOError;
    case nearby::sharing::Status::kEndpointUnknown:
      return Facade::Status::kEndpointUnknown;
    case nearby::sharing::Status::kConnectionRejected:
      return Facade::Status::kConnectionRejected;
    case nearby::sharing::Status::kAlreadyConnectedToEndpoint:
      return Facade::Status::kAlreadyConnectedToEndpoint;
    case nearby::sharing::Status::kNotConnectedToEndpoint:
      return Facade::Status::kNotConnectedToEndpoint;
    case nearby::sharing::Status::kBluetoothError:
      return Facade::Status::kBluetoothError;
    case nearby::sharing::Status::kBleError:
      return Facade::Status::kBleError;
    case nearby::sharing::Status::kWifiLanError:
      return Facade::Status::kWifiLanError;
    case nearby::sharing::Status::kPayloadUnknown:
      return Facade::Status::kPayloadUnknown;
    case nearby::sharing::Status::kReset:
      return Facade::Status::kReset;
    case nearby::sharing::Status::kTimeout:
      return Facade::Status::kTimeout;
    case nearby::sharing::Status::kUnknown:
      return Facade::Status::kUnknown;
    case nearby::sharing::Status::kNextValue:
      return Facade::Status::kNextValue;
  }
  return Facade::Status::kUnknown;
}

Facade::PayloadStatus ToFacadePayloadStatus(nearby::sharing::PayloadStatus status) {
  switch (status) {
    case nearby::sharing::kSuccess:
      return Facade::PayloadStatus::kSuccess;
    case nearby::sharing::kFailure:
      return Facade::PayloadStatus::kFailure;
    case nearby::sharing::kInProgress:
      return Facade::PayloadStatus::kInProgress;
    case nearby::sharing::kCanceled:
      return Facade::PayloadStatus::kCanceled;
  }
  return Facade::PayloadStatus::kFailure;
}

Facade::Medium ToFacadeMedium(nearby::sharing::Medium medium) {
  switch (medium) {
    case nearby::sharing::Medium::kUnknown:
      return Facade::Medium::kUnknown;
    case nearby::sharing::Medium::kMdns:
      return Facade::Medium::kMdns;
    case nearby::sharing::Medium::kBluetooth:
      return Facade::Medium::kBluetooth;
    case nearby::sharing::Medium::kWifiHotspot:
      return Facade::Medium::kWifiHotspot;
    case nearby::sharing::Medium::kBle:
      return Facade::Medium::kBle;
    case nearby::sharing::Medium::kWifiLan:
      return Facade::Medium::kWifiLan;
    case nearby::sharing::Medium::kWifiAware:
      return Facade::Medium::kWifiAware;
    case nearby::sharing::Medium::kNfc:
      return Facade::Medium::kNfc;
    case nearby::sharing::Medium::kWifiDirect:
      return Facade::Medium::kWifiDirect;
    case nearby::sharing::Medium::kWebRtc:
      return Facade::Medium::kWebRtc;
    case nearby::sharing::Medium::kBleL2Cap:
      return Facade::Medium::kBleL2Cap;
  }
  return Facade::Medium::kUnknown;
}

Facade::DistanceInfo ToFacadeDistance(nearby::sharing::DistanceInfo distance) {
  switch (distance) {
    case nearby::sharing::DistanceInfo::kUnknown:
      return Facade::DistanceInfo::kUnknown;
    case nearby::sharing::DistanceInfo::kVeryClose:
      return Facade::DistanceInfo::kVeryClose;
    case nearby::sharing::DistanceInfo::kClose:
      return Facade::DistanceInfo::kClose;
    case nearby::sharing::DistanceInfo::kFar:
      return Facade::DistanceInfo::kFar;
  }
  return Facade::DistanceInfo::kUnknown;
}

nearby::sharing::Strategy ToNativeStrategy(Facade::Strategy strategy) {
  switch (strategy) {
    case Facade::Strategy::kP2pCluster:
      return nearby::sharing::Strategy::kP2pCluster;
    case Facade::Strategy::kP2pStar:
      return nearby::sharing::Strategy::kP2pStar;
    case Facade::Strategy::kP2pPointToPoint:
      return nearby::sharing::Strategy::kP2pPointToPoint;
  }
  return nearby::sharing::Strategy::kP2pCluster;
}

nearby::sharing::MediumSelection ToNativeMediumSelection(
    const Facade::MediumSelection& selection) {
  return nearby::sharing::MediumSelection(selection.bluetooth, selection.ble,
                                          selection.web_rtc,
                                          selection.wifi_lan,
                                          selection.wifi_hotspot);
}

nearby::sharing::AdvertisingOptions ToNativeAdvertisingOptions(
    const Facade::AdvertisingOptions& options) {
  nearby::sharing::AdvertisingOptions native;
  native.strategy = ToNativeStrategy(options.strategy);
  native.allowed_mediums = ToNativeMediumSelection(options.allowed_mediums);
  native.auto_upgrade_bandwidth = options.auto_upgrade_bandwidth;
  native.enforce_topology_constraints = options.enforce_topology_constraints;
  native.enable_bluetooth_listening = options.enable_bluetooth_listening;
  return native;
}

nearby::sharing::DiscoveryOptions ToNativeDiscoveryOptions(
    const Facade::DiscoveryOptions& options) {
  nearby::sharing::DiscoveryOptions native;
  native.strategy = ToNativeStrategy(options.strategy);
  native.allowed_mediums = ToNativeMediumSelection(options.allowed_mediums);
  return native;
}

nearby::sharing::ConnectionOptions ToNativeConnectionOptions(
    const Facade::ConnectionOptions& options) {
  nearby::sharing::ConnectionOptions native;
  native.allowed_mediums = ToNativeMediumSelection(options.allowed_mediums);
  native.non_disruptive_hotspot_mode = options.non_disruptive_hotspot_mode;
  return native;
}

std::function<void(nearby::sharing::Status)> ToNativeStatusCallback(
    std::function<void(Facade::Status)> callback) {
  if (!callback) {
    return {};
  }
  return [cb = std::move(callback)](nearby::sharing::Status status) {
    cb(ToFacadeStatus(status));
  };
}

NativeService::ConnectionListener ToNativeConnectionListener(
    Facade::ConnectionListener listener) {
  NativeService::ConnectionListener native;

  auto initiated_cb = std::move(listener.initiated_cb);
  if (initiated_cb) {
    native.initiated_cb =
        [cb = std::move(initiated_cb)](const std::string& endpoint_id,
                                       const nearby::sharing::ConnectionInfo&
                                           info) mutable {
          Facade::ConnectionInfo translated;
          translated.is_incoming_connection = info.is_incoming_connection;
          translated.endpoint_info = info.endpoint_info;
          translated.peer_name = DecodePeerName(translated.endpoint_info);
          cb(endpoint_id, std::move(translated));
        };
  }

  auto accepted_cb = std::move(listener.accepted_cb);
  if (accepted_cb) {
    native.accepted_cb = [cb = std::move(accepted_cb)](
                             const std::string& endpoint_id) mutable {
      cb(endpoint_id);
    };
  }

  auto rejected_cb = std::move(listener.rejected_cb);
  if (rejected_cb) {
    native.rejected_cb = [cb = std::move(rejected_cb)](
                             const std::string& endpoint_id,
                             nearby::sharing::Status status) mutable {
      cb(endpoint_id, ToFacadeStatus(status));
    };
  }

  auto disconnected_cb = std::move(listener.disconnected_cb);
  if (disconnected_cb) {
    native.disconnected_cb = [cb = std::move(disconnected_cb)](
                                 const std::string& endpoint_id) mutable {
      cb(endpoint_id);
    };
  }

  auto bandwidth_changed_cb = std::move(listener.bandwidth_changed_cb);
  if (bandwidth_changed_cb) {
    native.bandwidth_changed_cb =
        [cb = std::move(bandwidth_changed_cb)](const std::string& endpoint_id,
                                               nearby::sharing::Medium medium)
            mutable { cb(endpoint_id, ToFacadeMedium(medium)); };
  }

  return native;
}

NativeService::DiscoveryListener ToNativeDiscoveryListener(
    Facade::DiscoveryListener listener) {
  NativeService::DiscoveryListener native;

  auto endpoint_found_cb = std::move(listener.endpoint_found_cb);
  if (endpoint_found_cb) {
    native.endpoint_found_cb =
        [cb = std::move(endpoint_found_cb)](
            const std::string& endpoint_id,
            const nearby::sharing::DiscoveredEndpointInfo& info) mutable {
          Facade::DiscoveredEndpointInfo translated;
          translated.endpoint_info = info.endpoint_info;
          translated.service_id = info.service_id;
          translated.peer_name = DecodePeerName(translated.endpoint_info);
          cb(endpoint_id, std::move(translated));
        };
  }

  auto endpoint_lost_cb = std::move(listener.endpoint_lost_cb);
  if (endpoint_lost_cb) {
    native.endpoint_lost_cb = [cb = std::move(endpoint_lost_cb)](
                                  const std::string& endpoint_id) mutable {
      cb(endpoint_id);
    };
  }

  auto endpoint_distance_changed_cb =
      std::move(listener.endpoint_distance_changed_cb);
  if (endpoint_distance_changed_cb) {
    native.endpoint_distance_changed_cb =
        [cb = std::move(endpoint_distance_changed_cb)](
            const std::string& endpoint_id,
            nearby::sharing::DistanceInfo distance_info) mutable {
          cb(endpoint_id, ToFacadeDistance(distance_info));
        };
  }

  return native;
}

NativeService::PayloadListener ToNativePayloadListener(
    Facade::PayloadListener listener) {
  NativeService::PayloadListener native;

  auto payload_cb = std::move(listener.payload_cb);
  if (payload_cb) {
    native.payload_cb =
        [cb = std::move(payload_cb)](absl::string_view endpoint_id,
                                     nearby::sharing::Payload payload) mutable {
          Facade::Payload translated;
          translated.id = payload.id;
          if (payload.content.is_bytes()) {
            translated.type = Facade::Payload::Type::kBytes;
            translated.bytes = std::move(payload.content.bytes_payload.bytes);
          } else if (payload.content.is_file()) {
            translated.type = Facade::Payload::Type::kFile;
            translated.parent_folder = payload.content.file_payload.parent_folder;
            translated.file_path =
                payload.content.file_payload.file_path.ToString();
            translated.file_name = payload.content.file_payload.file_path.GetFileName().ToString();
          }
          cb(std::string(endpoint_id), std::move(translated));
        };
  }

  auto progress_cb = std::move(listener.payload_progress_cb);
  if (progress_cb) {
    native.payload_progress_cb = [cb = std::move(progress_cb)](
                                     absl::string_view endpoint_id,
                                     const nearby::sharing::PayloadTransferUpdate&
                                         update) mutable {
      cb(std::string(endpoint_id),
         Facade::PayloadTransferUpdate{
             update.payload_id,
             ToFacadePayloadStatus(update.status),
             update.total_bytes,
             update.bytes_transferred,
         });
    };
  }

  return native;
}

std::unique_ptr<nearby::sharing::Payload> ToNativePayload(Facade::Payload payload) {
  const int64_t payload_id = payload.id == 0 ? NextPayloadId() : payload.id;
  switch (payload.type) {
    case Facade::Payload::Type::kBytes:
      return std::make_unique<nearby::sharing::Payload>(
          payload_id, std::move(payload.bytes));
    case Facade::Payload::Type::kFile:
      return std::make_unique<nearby::sharing::Payload>(
          payload_id, FilePath(payload.file_path), payload.parent_folder);
    case Facade::Payload::Type::kUnknown:
      return nullptr;
  }
  return nullptr;
}

}  // namespace

class NearbyConnectionsQtFacade::Impl {
 public:
  linux::NearbyConnectionsServiceLinux service;
};

NearbyConnectionsQtFacade::NearbyConnectionsQtFacade()
    : impl_(std::make_unique<Impl>()) {}

NearbyConnectionsQtFacade::~NearbyConnectionsQtFacade() = default;

NearbyConnectionsQtFacade::NearbyConnectionsQtFacade(
    NearbyConnectionsQtFacade&&) noexcept = default;

NearbyConnectionsQtFacade& NearbyConnectionsQtFacade::operator=(
    NearbyConnectionsQtFacade&&) noexcept = default;

void NearbyConnectionsQtFacade::SetBleL2capFlagOverrides(
    bool enable_ble_l2cap, bool refactor_ble_l2cap) {
  nearby::NearbyFlags::GetInstance().OverrideBoolFlagValue(
      nearby::connections::config_package_nearby::nearby_connections_feature::
          kEnableBleL2cap,
      enable_ble_l2cap);
  nearby::NearbyFlags::GetInstance().OverrideBoolFlagValue(
      nearby::connections::config_package_nearby::nearby_connections_feature::
          kRefactorBleL2cap,
      refactor_ble_l2cap);
}

NearbyConnectionsQtFacade::Payload NearbyConnectionsQtFacade::CreateBytesPayload(
    std::vector<uint8_t> bytes) const {
  Payload payload;
  payload.id = NextPayloadId();
  payload.type = Payload::Type::kBytes;
  payload.bytes = std::move(bytes);
  return payload;
}

void NearbyConnectionsQtFacade::StartAdvertising(
    const std::string& service_id, const std::vector<uint8_t>& endpoint_info,
    const AdvertisingOptions& advertising_options,
    ConnectionListener advertising_listener,
    std::function<void(Status)> callback) {
  impl_->service.StartAdvertising(
      service_id, endpoint_info, ToNativeAdvertisingOptions(advertising_options),
      ToNativeConnectionListener(std::move(advertising_listener)),
      ToNativeStatusCallback(std::move(callback)));
}

void NearbyConnectionsQtFacade::StopAdvertising(
    const std::string& service_id, std::function<void(Status)> callback) {
  impl_->service.StopAdvertising(service_id,
                                 ToNativeStatusCallback(std::move(callback)));
}

void NearbyConnectionsQtFacade::StartDiscovery(
    const std::string& service_id, const DiscoveryOptions& discovery_options,
    DiscoveryListener discovery_listener, std::function<void(Status)> callback) {
  impl_->service.StartDiscovery(
      service_id, ToNativeDiscoveryOptions(discovery_options),
      ToNativeDiscoveryListener(std::move(discovery_listener)),
      ToNativeStatusCallback(std::move(callback)));
}

void NearbyConnectionsQtFacade::StopDiscovery(const std::string& service_id,
                                              std::function<void(Status)> callback) {
  impl_->service.StopDiscovery(service_id,
                               ToNativeStatusCallback(std::move(callback)));
}

void NearbyConnectionsQtFacade::RequestConnection(
    const std::string& service_id, const std::vector<uint8_t>& endpoint_info,
    const std::string& endpoint_id, const ConnectionOptions& connection_options,
    ConnectionListener connection_listener, std::function<void(Status)> callback) {
  impl_->service.RequestConnection(
      service_id, endpoint_info, endpoint_id,
      ToNativeConnectionOptions(connection_options),
      ToNativeConnectionListener(std::move(connection_listener)),
      ToNativeStatusCallback(std::move(callback)));
}

void NearbyConnectionsQtFacade::DisconnectFromEndpoint(
    const std::string& service_id, const std::string& endpoint_id,
    std::function<void(Status)> callback) {
  impl_->service.DisconnectFromEndpoint(
      service_id, endpoint_id, ToNativeStatusCallback(std::move(callback)));
}

void NearbyConnectionsQtFacade::SendPayload(
    const std::string& service_id, const std::vector<std::string>& endpoint_ids,
    Payload payload, std::function<void(Status)> callback) {
  auto native_payload = ToNativePayload(std::move(payload));
  if (!native_payload) {
    if (callback) {
      callback(Status::kError);
    }
    return;
  }
  impl_->service.SendPayload(service_id, endpoint_ids, std::move(native_payload),
                             ToNativeStatusCallback(std::move(callback)));
}

void NearbyConnectionsQtFacade::InitiateBandwidthUpgrade(
    const std::string& service_id, const std::string& endpoint_id,
    std::function<void(Status)> callback) {
  impl_->service.InitiateBandwidthUpgrade(
      service_id, endpoint_id, ToNativeStatusCallback(std::move(callback)));
}

void NearbyConnectionsQtFacade::AcceptConnection(
    const std::string& service_id, const std::string& endpoint_id,
    PayloadListener payload_listener, std::function<void(Status)> callback) {
  impl_->service.AcceptConnection(
      service_id, endpoint_id, ToNativePayloadListener(std::move(payload_listener)),
      ToNativeStatusCallback(std::move(callback)));
}

void NearbyConnectionsQtFacade::StopAllEndpoints(
    std::function<void(Status)> callback) {
  impl_->service.StopAllEndpoints(ToNativeStatusCallback(std::move(callback)));
}

}  // namespace nearby::sharing::linux
