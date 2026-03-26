// Copyright 2026
//
// Thin app-facing API for NearbyConnectionsService on Linux that avoids
// exposing internal Nearby headers to external consumers.

#ifndef SHARING_LINUX_NEARBY_CONNECTIONS_API_H_
#define SHARING_LINUX_NEARBY_CONNECTIONS_API_H_

#include <stdint.h>

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace nearby {
namespace sharing {

class __attribute__((visibility("default"))) NearbyConnectionsApi {
 public:
  enum class StatusCode {
    kSuccess = 0,
    kError = 1,
    kOutOfOrderApiCall = 2,
    kAlreadyHaveActiveStrategy = 3,
    kAlreadyAdvertising = 4,
    kAlreadyDiscovering = 5,
    kAlreadyListening = 6,
    kEndpointIOError = 7,
    kEndpointUnknown = 8,
    kConnectionRejected = 9,
    kAlreadyConnectedToEndpoint = 10,
    kNotConnectedToEndpoint = 11,
    kBluetoothError = 12,
    kBleError = 13,
    kWifiLanError = 14,
    kPayloadUnknown = 15,
    kReset = 16,
    kTimeout = 17,
    kUnknown = 18,
  };

  enum class Strategy {
    kP2pCluster = 0,
    kP2pStar = 1,
    kP2pPointToPoint = 2,
  };

  enum class Medium {
    kUnknown = 0,
    kMdns = 1,
    kBluetooth = 2,
    kWifiHotspot = 3,
    kBle = 4,
    kWifiLan = 5,
    kWifiAware = 6,
    kNfc = 7,
    kWifiDirect = 8,
    kWebRtc = 9,
    kBleL2Cap = 10,
  };

  enum class DistanceInfo {
    kUnknown = 1,
    kVeryClose = 2,
    kClose = 3,
    kFar = 4,
  };

  enum class AuthenticationStatus {
    kUnknown = 0,
    kSuccess = 1,
    kFailure = 2,
  };

  enum class PayloadType {
    kUnknown = 0,
    kBytes = 1,
    kFile = 2,
  };

  enum class PayloadStatus {
    kSuccess = 0,
    kFailure = 1,
    kInProgress = 2,
    kCanceled = 3,
  };

  struct Uuid {
    std::string uuid;
  };

  struct MediumSelection {
    bool bluetooth = true;
    bool ble = true;
    bool web_rtc = true;
    bool wifi_lan = true;
    bool wifi_hotspot = true;
  };

  struct AdvertisingOptions {
    Strategy strategy = Strategy::kP2pCluster;
    MediumSelection allowed_mediums;
    bool auto_upgrade_bandwidth = true;
    bool enforce_topology_constraints = true;
    bool enable_bluetooth_listening = false;
    bool enable_webrtc_listening = false;
    bool use_stable_endpoint_id = false;
    bool force_new_endpoint_id = false;
    std::string fast_advertisement_service_uuid;
  };

  struct DiscoveryOptions {
    Strategy strategy = Strategy::kP2pCluster;
    MediumSelection allowed_mediums;
    bool has_fast_advertisement_service_uuid = false;
    Uuid fast_advertisement_service_uuid;
    bool is_out_of_band_connection = false;
    bool has_alternate_service_uuid = false;
    uint16_t alternate_service_uuid = 0;
  };

  struct ConnectionOptions {
    MediumSelection allowed_mediums;
    std::vector<uint8_t> remote_bluetooth_mac_address;
    bool has_keep_alive_interval_millis = false;
    int64_t keep_alive_interval_millis = 0;
    bool has_keep_alive_timeout_millis = false;
    int64_t keep_alive_timeout_millis = 0;
    bool non_disruptive_hotspot_mode = false;
  };

  struct ConnectionInfo {
    std::string authentication_token;
    std::vector<uint8_t> raw_authentication_token;
    std::vector<uint8_t> endpoint_info;
    bool is_incoming_connection = false;
    StatusCode connection_layer_status = StatusCode::kUnknown;
    AuthenticationStatus authentication_status =
        AuthenticationStatus::kUnknown;
  };

  struct DiscoveredEndpointInfo {
    std::vector<uint8_t> endpoint_info;
    std::string service_id;
  };

  struct PayloadTransferUpdate {
    int64_t payload_id = 0;
    PayloadStatus status = PayloadStatus::kInProgress;
    uint64_t total_bytes = 0;
    uint64_t bytes_transferred = 0;
  };

  struct Payload {
    int64_t id = 0;
    PayloadType type = PayloadType::kUnknown;
    std::vector<uint8_t> bytes;
    std::string file_path;
    std::string parent_folder;

    static Payload FromBytes(int64_t id, std::vector<uint8_t> bytes) {
      Payload payload;
      payload.id = id;
      payload.type = PayloadType::kBytes;
      payload.bytes = std::move(bytes);
      return payload;
    }

    static Payload FromFile(int64_t id, std::string file_path,
                            std::string parent_folder = {}) {
      Payload payload;
      payload.id = id;
      payload.type = PayloadType::kFile;
      payload.file_path = std::move(file_path);
      payload.parent_folder = std::move(parent_folder);
      return payload;
    }
  };

  struct Listener {
    std::function<void(const std::string&, const DiscoveredEndpointInfo&)>
        endpoint_found_cb;
    std::function<void(const std::string&)> endpoint_lost_cb;
    std::function<void(const std::string&, DistanceInfo)>
        endpoint_distance_changed_cb;

    std::function<void(const std::string&, const ConnectionInfo&)>
        connection_initiated_cb;
    std::function<void(const std::string&)> connection_accepted_cb;
    std::function<void(const std::string&, StatusCode)> connection_rejected_cb;
    std::function<void(const std::string&)> disconnected_cb;
    std::function<void(const std::string&, Medium)> bandwidth_changed_cb;

    std::function<void(const std::string&, const Payload&)>
        payload_received_cb;
    std::function<void(const std::string&, const PayloadTransferUpdate&)>
        payload_transfer_update_cb;
  };

  NearbyConnectionsApi();
  ~NearbyConnectionsApi();

  NearbyConnectionsApi(const NearbyConnectionsApi&) = delete;
  NearbyConnectionsApi& operator=(const NearbyConnectionsApi&) = delete;
  NearbyConnectionsApi(NearbyConnectionsApi&&) noexcept;
  NearbyConnectionsApi& operator=(NearbyConnectionsApi&&) noexcept;

  void SetListener(Listener listener);

  void StartAdvertising(const std::string& service_id,
                        const std::vector<uint8_t>& endpoint_info,
                        const AdvertisingOptions& options,
                        std::function<void(StatusCode)> callback);
  void StopAdvertising(const std::string& service_id,
                       std::function<void(StatusCode)> callback);

  void StartDiscovery(const std::string& service_id,
                      const DiscoveryOptions& options,
                      std::function<void(StatusCode)> callback);
  void StopDiscovery(const std::string& service_id,
                     std::function<void(StatusCode)> callback);

  void RequestConnection(const std::string& service_id,
                         const std::vector<uint8_t>& endpoint_info,
                         const std::string& endpoint_id,
                         const ConnectionOptions& options,
                         std::function<void(StatusCode)> callback);

  void DisconnectFromEndpoint(const std::string& service_id,
                              const std::string& endpoint_id,
                              std::function<void(StatusCode)> callback);

  void SendPayload(const std::string& service_id,
                   const std::vector<std::string>& endpoint_ids,
                   Payload payload,
                   std::function<void(StatusCode)> callback);
  void CancelPayload(const std::string& service_id, int64_t payload_id,
                     std::function<void(StatusCode)> callback);

  void InitiateBandwidthUpgrade(const std::string& service_id,
                                const std::string& endpoint_id,
                                std::function<void(StatusCode)> callback);

  void AcceptConnection(const std::string& service_id,
                        const std::string& endpoint_id,
                        std::function<void(StatusCode)> callback);

  void StopAllEndpoints(std::function<void(StatusCode)> callback);

  void SetCustomSavePath(const std::string& path,
                         std::function<void(StatusCode)> callback);
  void OverrideSavePath(const std::string& endpoint_id,
                        const std::string& path);

  std::string Dump() const;

  static std::string StatusCodeToString(StatusCode status);

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace sharing
}  // namespace nearby

#endif  // SHARING_LINUX_NEARBY_CONNECTIONS_API_H_
