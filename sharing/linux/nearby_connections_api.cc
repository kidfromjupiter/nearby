#include "sharing/linux/nearby_connections_api.h"

#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "absl/time/time.h"
#include "internal/base/file_path.h"
#include "internal/interop/authentication_status.h"
#include "sharing/internal/public/context_impl.h"
#include "sharing/linux/platform/linux_sharing_platform.h"
#include "sharing/nearby_connections_service.h"
#include "sharing/nearby_connections_service_impl.h"

namespace nearby::sharing {

namespace {

using NativeService = nearby::sharing::NearbyConnectionsService;

NearbyConnectionsApi::StatusCode ToFacadeStatus(Status status) {
  switch (status) {
    case Status::kSuccess:
      return NearbyConnectionsApi::StatusCode::kSuccess;
    case Status::kError:
      return NearbyConnectionsApi::StatusCode::kError;
    case Status::kOutOfOrderApiCall:
      return NearbyConnectionsApi::StatusCode::kOutOfOrderApiCall;
    case Status::kAlreadyHaveActiveStrategy:
      return NearbyConnectionsApi::StatusCode::kAlreadyHaveActiveStrategy;
    case Status::kAlreadyAdvertising:
      return NearbyConnectionsApi::StatusCode::kAlreadyAdvertising;
    case Status::kAlreadyDiscovering:
      return NearbyConnectionsApi::StatusCode::kAlreadyDiscovering;
    case Status::kAlreadyListening:
      return NearbyConnectionsApi::StatusCode::kAlreadyListening;
    case Status::kEndpointIOError:
      return NearbyConnectionsApi::StatusCode::kEndpointIOError;
    case Status::kEndpointUnknown:
      return NearbyConnectionsApi::StatusCode::kEndpointUnknown;
    case Status::kConnectionRejected:
      return NearbyConnectionsApi::StatusCode::kConnectionRejected;
    case Status::kAlreadyConnectedToEndpoint:
      return NearbyConnectionsApi::StatusCode::kAlreadyConnectedToEndpoint;
    case Status::kNotConnectedToEndpoint:
      return NearbyConnectionsApi::StatusCode::kNotConnectedToEndpoint;
    case Status::kBluetoothError:
      return NearbyConnectionsApi::StatusCode::kBluetoothError;
    case Status::kBleError:
      return NearbyConnectionsApi::StatusCode::kBleError;
    case Status::kWifiLanError:
      return NearbyConnectionsApi::StatusCode::kWifiLanError;
    case Status::kPayloadUnknown:
      return NearbyConnectionsApi::StatusCode::kPayloadUnknown;
    case Status::kReset:
      return NearbyConnectionsApi::StatusCode::kReset;
    case Status::kTimeout:
      return NearbyConnectionsApi::StatusCode::kTimeout;
    case Status::kUnknown:
      return NearbyConnectionsApi::StatusCode::kUnknown;
    case Status::kNextValue:
      break;
  }
  return NearbyConnectionsApi::StatusCode::kUnknown;
}

NearbyConnectionsApi::AuthenticationStatus ToFacadeAuthenticationStatus(
    ::nearby::AuthenticationStatus status) {
  switch (status) {
    case ::nearby::AuthenticationStatus::kUnknown:
      return NearbyConnectionsApi::AuthenticationStatus::kUnknown;
    case ::nearby::AuthenticationStatus::kSuccess:
      return NearbyConnectionsApi::AuthenticationStatus::kSuccess;
    case ::nearby::AuthenticationStatus::kFailure:
      return NearbyConnectionsApi::AuthenticationStatus::kFailure;
  }
  return NearbyConnectionsApi::AuthenticationStatus::kUnknown;
}

NearbyConnectionsApi::DistanceInfo ToFacadeDistanceInfo(
    nearby::sharing::DistanceInfo distance_info) {
  switch (distance_info) {
    case nearby::sharing::DistanceInfo::kUnknown:
      return NearbyConnectionsApi::DistanceInfo::kUnknown;
    case nearby::sharing::DistanceInfo::kVeryClose:
      return NearbyConnectionsApi::DistanceInfo::kVeryClose;
    case nearby::sharing::DistanceInfo::kClose:
      return NearbyConnectionsApi::DistanceInfo::kClose;
    case nearby::sharing::DistanceInfo::kFar:
      return NearbyConnectionsApi::DistanceInfo::kFar;
  }
  return NearbyConnectionsApi::DistanceInfo::kUnknown;
}

NearbyConnectionsApi::Medium ToFacadeMedium(nearby::sharing::Medium medium) {
  switch (medium) {
    case nearby::sharing::Medium::kUnknown:
      return NearbyConnectionsApi::Medium::kUnknown;
    case nearby::sharing::Medium::kMdns:
      return NearbyConnectionsApi::Medium::kMdns;
    case nearby::sharing::Medium::kBluetooth:
      return NearbyConnectionsApi::Medium::kBluetooth;
    case nearby::sharing::Medium::kWifiHotspot:
      return NearbyConnectionsApi::Medium::kWifiHotspot;
    case nearby::sharing::Medium::kBle:
      return NearbyConnectionsApi::Medium::kBle;
    case nearby::sharing::Medium::kWifiLan:
      return NearbyConnectionsApi::Medium::kWifiLan;
    case nearby::sharing::Medium::kWifiAware:
      return NearbyConnectionsApi::Medium::kWifiAware;
    case nearby::sharing::Medium::kNfc:
      return NearbyConnectionsApi::Medium::kNfc;
    case nearby::sharing::Medium::kWifiDirect:
      return NearbyConnectionsApi::Medium::kWifiDirect;
    case nearby::sharing::Medium::kWebRtc:
      return NearbyConnectionsApi::Medium::kWebRtc;
    case nearby::sharing::Medium::kBleL2Cap:
      return NearbyConnectionsApi::Medium::kBleL2Cap;
  }
  return NearbyConnectionsApi::Medium::kUnknown;
}

nearby::sharing::Strategy ToNativeStrategy(
    NearbyConnectionsApi::Strategy strategy) {
  switch (strategy) {
    case NearbyConnectionsApi::Strategy::kP2pCluster:
      return nearby::sharing::Strategy::kP2pCluster;
    case NearbyConnectionsApi::Strategy::kP2pStar:
      return nearby::sharing::Strategy::kP2pStar;
    case NearbyConnectionsApi::Strategy::kP2pPointToPoint:
      return nearby::sharing::Strategy::kP2pPointToPoint;
  }
  return nearby::sharing::Strategy::kP2pCluster;
}

nearby::sharing::MediumSelection ToNativeMediumSelection(
    const NearbyConnectionsApi::MediumSelection& selection) {
  nearby::sharing::MediumSelection native_selection;
  native_selection.bluetooth = selection.bluetooth;
  native_selection.ble = selection.ble;
  native_selection.web_rtc = selection.web_rtc;
  native_selection.wifi_lan = selection.wifi_lan;
  native_selection.wifi_hotspot = selection.wifi_hotspot;
  return native_selection;
}

nearby::sharing::AdvertisingOptions ToNativeAdvertisingOptions(
    const NearbyConnectionsApi::AdvertisingOptions& options) {
  nearby::sharing::AdvertisingOptions native_options;
  native_options.strategy = ToNativeStrategy(options.strategy);
  native_options.allowed_mediums =
      ToNativeMediumSelection(options.allowed_mediums);
  native_options.auto_upgrade_bandwidth = options.auto_upgrade_bandwidth;
  native_options.enforce_topology_constraints =
      options.enforce_topology_constraints;
  native_options.enable_bluetooth_listening =
      options.enable_bluetooth_listening;
  native_options.enable_webrtc_listening = options.enable_webrtc_listening;
  native_options.use_stable_endpoint_id = options.use_stable_endpoint_id;
  native_options.force_new_endpoint_id = options.force_new_endpoint_id;
  native_options.fast_advertisement_service_uuid =
      nearby::sharing::Uuid(options.fast_advertisement_service_uuid);
  return native_options;
}

nearby::sharing::DiscoveryOptions ToNativeDiscoveryOptions(
    const NearbyConnectionsApi::DiscoveryOptions& options) {
  nearby::sharing::DiscoveryOptions native_options;
  native_options.strategy = ToNativeStrategy(options.strategy);
  native_options.allowed_mediums =
      ToNativeMediumSelection(options.allowed_mediums);
  if (options.has_fast_advertisement_service_uuid) {
    native_options.fast_advertisement_service_uuid =
        nearby::sharing::Uuid(options.fast_advertisement_service_uuid.uuid);
  }
  native_options.is_out_of_band_connection =
      options.is_out_of_band_connection;
  if (options.has_alternate_service_uuid) {
    native_options.alternate_service_uuid = options.alternate_service_uuid;
  }
  return native_options;
}

nearby::sharing::ConnectionOptions ToNativeConnectionOptions(
    const NearbyConnectionsApi::ConnectionOptions& options) {
  nearby::sharing::ConnectionOptions native_options;
  native_options.allowed_mediums =
      ToNativeMediumSelection(options.allowed_mediums);
  if (!options.remote_bluetooth_mac_address.empty()) {
    native_options.remote_bluetooth_mac_address =
        options.remote_bluetooth_mac_address;
  }
  if (options.has_keep_alive_interval_millis &&
      options.keep_alive_interval_millis >= 0) {
    native_options.keep_alive_interval =
        absl::Milliseconds(options.keep_alive_interval_millis);
  }
  if (options.has_keep_alive_timeout_millis &&
      options.keep_alive_timeout_millis >= 0) {
    native_options.keep_alive_timeout =
        absl::Milliseconds(options.keep_alive_timeout_millis);
  }
  native_options.non_disruptive_hotspot_mode =
      options.non_disruptive_hotspot_mode;
  return native_options;
}

NearbyConnectionsApi::ConnectionInfo ToFacadeConnectionInfo(
    const nearby::sharing::ConnectionInfo& info) {
  NearbyConnectionsApi::ConnectionInfo facade_info;
  facade_info.authentication_token = info.authentication_token;
  facade_info.raw_authentication_token = info.raw_authentication_token;
  facade_info.endpoint_info = info.endpoint_info;
  facade_info.is_incoming_connection = info.is_incoming_connection;
  facade_info.connection_layer_status =
      ToFacadeStatus(info.connection_layer_status);
  facade_info.authentication_status =
      ToFacadeAuthenticationStatus(info.authentication_status);
  return facade_info;
}

NearbyConnectionsApi::DiscoveredEndpointInfo ToFacadeDiscoveredEndpointInfo(
    const nearby::sharing::DiscoveredEndpointInfo& info) {
  NearbyConnectionsApi::DiscoveredEndpointInfo facade_info;
  facade_info.endpoint_info = info.endpoint_info;
  facade_info.service_id = info.service_id;
  return facade_info;
}

NearbyConnectionsApi::PayloadStatus ToFacadePayloadStatus(
    nearby::sharing::PayloadStatus status) {
  switch (status) {
    case nearby::sharing::PayloadStatus::kSuccess:
      return NearbyConnectionsApi::PayloadStatus::kSuccess;
    case nearby::sharing::PayloadStatus::kFailure:
      return NearbyConnectionsApi::PayloadStatus::kFailure;
    case nearby::sharing::PayloadStatus::kInProgress:
      return NearbyConnectionsApi::PayloadStatus::kInProgress;
    case nearby::sharing::PayloadStatus::kCanceled:
      return NearbyConnectionsApi::PayloadStatus::kCanceled;
  }
  return NearbyConnectionsApi::PayloadStatus::kFailure;
}

NearbyConnectionsApi::Payload ToFacadePayload(
    const nearby::sharing::Payload& payload) {
  NearbyConnectionsApi::Payload facade_payload;
  facade_payload.id = payload.id;
  switch (payload.content.type) {
    case nearby::sharing::PayloadContent::Type::kBytes:
      facade_payload.type = NearbyConnectionsApi::PayloadType::kBytes;
      facade_payload.bytes = payload.content.bytes_payload.bytes;
      break;
    case nearby::sharing::PayloadContent::Type::kStream:
      facade_payload.type = NearbyConnectionsApi::PayloadType::kStream;
      facade_payload.stream_bytes = payload.content.stream_payload.bytes;
      break;
    case nearby::sharing::PayloadContent::Type::kFile:
      facade_payload.type = NearbyConnectionsApi::PayloadType::kFile;
      facade_payload.file_path =
          payload.content.file_payload.file_path.ToString();
      facade_payload.parent_folder = payload.content.file_payload.parent_folder;
      break;
    case nearby::sharing::PayloadContent::Type::kUnknown:
      facade_payload.type = NearbyConnectionsApi::PayloadType::kUnknown;
      break;
  }
  return facade_payload;
}

std::unique_ptr<nearby::sharing::Payload> ToNativePayload(
    NearbyConnectionsApi::Payload payload) {
  switch (payload.type) {
    case NearbyConnectionsApi::PayloadType::kBytes:
      return std::make_unique<nearby::sharing::Payload>(
          payload.id, std::move(payload.bytes));
    case NearbyConnectionsApi::PayloadType::kFile:
      return std::make_unique<nearby::sharing::Payload>(
          payload.id, FilePath(payload.file_path), payload.parent_folder);
    case NearbyConnectionsApi::PayloadType::kStream: {
      nearby::sharing::StreamPayload stream_payload;
      stream_payload.bytes = std::move(payload.stream_bytes);
      stream_payload.input_stream = std::move(payload.stream_input);
      return std::make_unique<nearby::sharing::Payload>(payload.id,
                                                        std::move(stream_payload));
    }
    case NearbyConnectionsApi::PayloadType::kUnknown:
      return nullptr;
  }
  return nullptr;
}

NearbyConnectionsApi::PayloadTransferUpdate ToFacadePayloadTransferUpdate(
    const nearby::sharing::PayloadTransferUpdate& update) {
  NearbyConnectionsApi::PayloadTransferUpdate facade_update;
  facade_update.payload_id = update.payload_id;
  facade_update.status = ToFacadePayloadStatus(update.status);
  facade_update.total_bytes = update.total_bytes;
  facade_update.bytes_transferred = update.bytes_transferred;
  return facade_update;
}

}  // namespace

class NearbyConnectionsApi::Impl {
 public:
  struct ListenerState {
    std::mutex mutex;
    NearbyConnectionsApi::Listener listener;
  };

  Impl()
      : platform(),
        context(platform),
        service(std::make_unique<NearbyConnectionsServiceImpl>(
            context.GetConnectivityManager(), /*event_logger=*/nullptr)),
        listener_state(std::make_shared<ListenerState>()) {}

  static NearbyConnectionsApi::Listener CopyListener(
      const std::shared_ptr<ListenerState>& listener_state) {
    std::scoped_lock lock(listener_state->mutex);
    return listener_state->listener;
  }

  static NativeService::ConnectionListener BuildConnectionListener(
      const std::shared_ptr<ListenerState>& listener_state) {
    NativeService::ConnectionListener listener;
    listener.initiated_cb =
        [listener_state](const std::string& endpoint_id,
                         const nearby::sharing::ConnectionInfo& info) {
          NearbyConnectionsApi::Listener listener_copy =
              CopyListener(listener_state);
          if (!listener_copy.connection_initiated_cb) {
            return;
          }
          listener_copy.connection_initiated_cb(endpoint_id,
                                                ToFacadeConnectionInfo(info));
        };
    listener.accepted_cb = [listener_state](const std::string& endpoint_id) {
      NearbyConnectionsApi::Listener listener_copy =
          CopyListener(listener_state);
      if (listener_copy.connection_accepted_cb) {
        listener_copy.connection_accepted_cb(endpoint_id);
      }
    };
    listener.rejected_cb = [listener_state](const std::string& endpoint_id,
                                            Status status) {
      NearbyConnectionsApi::Listener listener_copy =
          CopyListener(listener_state);
      if (listener_copy.connection_rejected_cb) {
        listener_copy.connection_rejected_cb(endpoint_id,
                                             ToFacadeStatus(status));
      }
    };
    listener.disconnected_cb = [listener_state](const std::string& endpoint_id) {
      NearbyConnectionsApi::Listener listener_copy =
          CopyListener(listener_state);
      if (listener_copy.disconnected_cb) {
        listener_copy.disconnected_cb(endpoint_id);
      }
    };
    listener.bandwidth_changed_cb =
        [listener_state](const std::string& endpoint_id,
                         nearby::sharing::Medium medium) {
          NearbyConnectionsApi::Listener listener_copy =
              CopyListener(listener_state);
          if (listener_copy.bandwidth_changed_cb) {
            listener_copy.bandwidth_changed_cb(endpoint_id,
                                               ToFacadeMedium(medium));
          }
        };
    return listener;
  }

  static NativeService::DiscoveryListener BuildDiscoveryListener(
      const std::shared_ptr<ListenerState>& listener_state) {
    NativeService::DiscoveryListener listener;
    listener.endpoint_found_cb =
        [listener_state](const std::string& endpoint_id,
                         const nearby::sharing::DiscoveredEndpointInfo& info) {
          NearbyConnectionsApi::Listener listener_copy =
              CopyListener(listener_state);
          if (listener_copy.endpoint_found_cb) {
            listener_copy.endpoint_found_cb(
                endpoint_id, ToFacadeDiscoveredEndpointInfo(info));
          }
        };
    listener.endpoint_lost_cb = [listener_state](const std::string& endpoint_id) {
      NearbyConnectionsApi::Listener listener_copy =
          CopyListener(listener_state);
      if (listener_copy.endpoint_lost_cb) {
        listener_copy.endpoint_lost_cb(endpoint_id);
      }
    };
    listener.endpoint_distance_changed_cb =
        [listener_state](const std::string& endpoint_id,
                         nearby::sharing::DistanceInfo distance_info) {
          NearbyConnectionsApi::Listener listener_copy =
              CopyListener(listener_state);
          if (listener_copy.endpoint_distance_changed_cb) {
            listener_copy.endpoint_distance_changed_cb(
                endpoint_id, ToFacadeDistanceInfo(distance_info));
          }
        };
    return listener;
  }

  static NativeService::PayloadListener BuildPayloadListener(
      const std::shared_ptr<ListenerState>& listener_state) {
    NativeService::PayloadListener listener;
    listener.payload_cb = [listener_state](absl::string_view endpoint_id,
                                           nearby::sharing::Payload payload) {
      NearbyConnectionsApi::Listener listener_copy =
          CopyListener(listener_state);
      if (listener_copy.payload_received_cb) {
        listener_copy.payload_received_cb(std::string(endpoint_id),
                                          ToFacadePayload(payload));
      }
    };
    listener.payload_progress_cb =
        [listener_state](
            absl::string_view endpoint_id,
            const nearby::sharing::PayloadTransferUpdate& update) {
          NearbyConnectionsApi::Listener listener_copy =
              CopyListener(listener_state);
          if (listener_copy.payload_transfer_update_cb) {
            listener_copy.payload_transfer_update_cb(
                std::string(endpoint_id), ToFacadePayloadTransferUpdate(update));
          }
        };
    return listener;
  }

  LinuxSharingPlatform platform;
  ::nearby::ContextImpl context;
  std::unique_ptr<NativeService> service;
  std::shared_ptr<ListenerState> listener_state;
};

NearbyConnectionsApi::NearbyConnectionsApi() : impl_(std::make_unique<Impl>()) {}

NearbyConnectionsApi::~NearbyConnectionsApi() = default;

NearbyConnectionsApi::NearbyConnectionsApi(NearbyConnectionsApi&&) noexcept =
    default;

NearbyConnectionsApi& NearbyConnectionsApi::operator=(
    NearbyConnectionsApi&&) noexcept = default;

void NearbyConnectionsApi::SetListener(Listener listener) {
  std::scoped_lock lock(impl_->listener_state->mutex);
  impl_->listener_state->listener = std::move(listener);
}

void NearbyConnectionsApi::StartAdvertising(
    const std::string& service_id, const std::vector<uint8_t>& endpoint_info,
    const AdvertisingOptions& options,
    std::function<void(StatusCode)> callback) {
  impl_->service->StartAdvertising(
      service_id, endpoint_info, ToNativeAdvertisingOptions(options),
      Impl::BuildConnectionListener(impl_->listener_state),
      [callback = std::move(callback)](Status status) mutable {
        if (callback) {
          callback(ToFacadeStatus(status));
        }
      });
}

void NearbyConnectionsApi::StopAdvertising(
    const std::string& service_id, std::function<void(StatusCode)> callback) {
  impl_->service->StopAdvertising(
      service_id, [callback = std::move(callback)](Status status) mutable {
        if (callback) {
          callback(ToFacadeStatus(status));
        }
      });
}

void NearbyConnectionsApi::StartDiscovery(
    const std::string& service_id, const DiscoveryOptions& options,
    std::function<void(StatusCode)> callback) {
  impl_->service->StartDiscovery(
      service_id, ToNativeDiscoveryOptions(options),
      Impl::BuildDiscoveryListener(impl_->listener_state),
      [callback = std::move(callback)](Status status) mutable {
        if (callback) {
          callback(ToFacadeStatus(status));
        }
      });
}

void NearbyConnectionsApi::StopDiscovery(
    const std::string& service_id, std::function<void(StatusCode)> callback) {
  impl_->service->StopDiscovery(
      service_id, [callback = std::move(callback)](Status status) mutable {
        if (callback) {
          callback(ToFacadeStatus(status));
        }
      });
}

void NearbyConnectionsApi::RequestConnection(
    const std::string& service_id, const std::vector<uint8_t>& endpoint_info,
    const std::string& endpoint_id, const ConnectionOptions& options,
    std::function<void(StatusCode)> callback) {
  impl_->service->RequestConnection(
      service_id, endpoint_info, endpoint_id, ToNativeConnectionOptions(options),
      Impl::BuildConnectionListener(impl_->listener_state),
      [callback = std::move(callback)](Status status) mutable {
        if (callback) {
          callback(ToFacadeStatus(status));
        }
      });
}

void NearbyConnectionsApi::DisconnectFromEndpoint(
    const std::string& service_id, const std::string& endpoint_id,
    std::function<void(StatusCode)> callback) {
  impl_->service->DisconnectFromEndpoint(
      service_id, endpoint_id,
      [callback = std::move(callback)](Status status) mutable {
        if (callback) {
          callback(ToFacadeStatus(status));
        }
      });
}

void NearbyConnectionsApi::SendPayload(
    const std::string& service_id, const std::vector<std::string>& endpoint_ids,
    Payload payload, std::function<void(StatusCode)> callback) {
  std::unique_ptr<nearby::sharing::Payload> native_payload =
      ToNativePayload(std::move(payload));
  if (native_payload == nullptr) {
    if (callback) {
      callback(StatusCode::kError);
    }
    return;
  }

  impl_->service->SendPayload(
      service_id, endpoint_ids, std::move(native_payload),
      [callback = std::move(callback)](Status status) mutable {
        if (callback) {
          callback(ToFacadeStatus(status));
        }
      });
}

void NearbyConnectionsApi::CancelPayload(
    const std::string& service_id, int64_t payload_id,
    std::function<void(StatusCode)> callback) {
  impl_->service->CancelPayload(
      service_id, payload_id,
      [callback = std::move(callback)](Status status) mutable {
        if (callback) {
          callback(ToFacadeStatus(status));
        }
      });
}

void NearbyConnectionsApi::InitiateBandwidthUpgrade(
    const std::string& service_id, const std::string& endpoint_id,
    std::function<void(StatusCode)> callback) {
  impl_->service->InitiateBandwidthUpgrade(
      service_id, endpoint_id,
      [callback = std::move(callback)](Status status) mutable {
        if (callback) {
          callback(ToFacadeStatus(status));
        }
      });
}

void NearbyConnectionsApi::AcceptConnection(
    const std::string& service_id, const std::string& endpoint_id,
    std::function<void(StatusCode)> callback) {
  impl_->service->AcceptConnection(
      service_id, endpoint_id, Impl::BuildPayloadListener(impl_->listener_state),
      [callback = std::move(callback)](Status status) mutable {
        if (callback) {
          callback(ToFacadeStatus(status));
        }
      });
}

void NearbyConnectionsApi::StopAllEndpoints(
    std::function<void(StatusCode)> callback) {
  impl_->service->StopAllEndpoints(
      [callback = std::move(callback)](Status status) mutable {
        if (callback) {
          callback(ToFacadeStatus(status));
        }
      });
}

void NearbyConnectionsApi::SetCustomSavePath(
    const std::string& path, std::function<void(StatusCode)> callback) {
  impl_->service->SetCustomSavePath(
      path, [callback = std::move(callback)](Status status) mutable {
        if (callback) {
          callback(ToFacadeStatus(status));
        }
      });
}

void NearbyConnectionsApi::OverrideSavePath(const std::string& endpoint_id,
                                            const std::string& path) {
  impl_->service->OverrideSavePath(endpoint_id, path);
}

std::string NearbyConnectionsApi::Dump() const { return impl_->service->Dump(); }

std::string NearbyConnectionsApi::StatusCodeToString(StatusCode status) {
  switch (status) {
    case StatusCode::kSuccess:
      return "Success";
    case StatusCode::kError:
      return "Error";
    case StatusCode::kOutOfOrderApiCall:
      return "OutOfOrderApiCall";
    case StatusCode::kAlreadyHaveActiveStrategy:
      return "AlreadyHaveActiveStrategy";
    case StatusCode::kAlreadyAdvertising:
      return "AlreadyAdvertising";
    case StatusCode::kAlreadyDiscovering:
      return "AlreadyDiscovering";
    case StatusCode::kAlreadyListening:
      return "AlreadyListening";
    case StatusCode::kEndpointIOError:
      return "EndpointIOError";
    case StatusCode::kEndpointUnknown:
      return "EndpointUnknown";
    case StatusCode::kConnectionRejected:
      return "ConnectionRejected";
    case StatusCode::kAlreadyConnectedToEndpoint:
      return "AlreadyConnectedToEndpoint";
    case StatusCode::kNotConnectedToEndpoint:
      return "NotConnectedToEndpoint";
    case StatusCode::kBluetoothError:
      return "BluetoothError";
    case StatusCode::kBleError:
      return "BleError";
    case StatusCode::kWifiLanError:
      return "WifiLanError";
    case StatusCode::kPayloadUnknown:
      return "PayloadUnknown";
    case StatusCode::kReset:
      return "Reset";
    case StatusCode::kTimeout:
      return "Timeout";
    case StatusCode::kUnknown:
      return "Unknown";
  }
  return "Unknown";
}

}  // namespace nearby::sharing
