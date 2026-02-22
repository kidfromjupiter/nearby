// Copyright 2026
//
// Thin C++ facade around NearbyConnectionsServiceLinux for UI clients that
// should not include internal Nearby/Abseil/Protobuf headers.

#ifndef SHARING_LINUX_NEARBY_CONNECTIONS_QT_FACADE_H_
#define SHARING_LINUX_NEARBY_CONNECTIONS_QT_FACADE_H_

#include <stdint.h>

#include <functional>
#include <memory>
#include <string>
#include <vector>

// Some toolchains define `linux` as a macro (e.g. `#define linux 1`), which
// breaks namespace tokens like `nearby::sharing::linux`.
#ifdef linux
#undef linux
#endif

namespace nearby {
namespace sharing {
class NearbyConnectionsQtFacade {
public:
  enum class Status {
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
    kNextValue = 19,
  };

  enum class PayloadStatus {
    kSuccess,
    kFailure,
    kInProgress,
    kCanceled,
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

  enum class Strategy {
    kP2pCluster = 0,
    kP2pStar = 1,
    kP2pPointToPoint = 2,
  };

  struct ConnectionInfo {
    bool is_incoming_connection = false;
    std::vector<uint8_t> endpoint_info;
    std::string peer_name;
  };

  struct DiscoveredEndpointInfo {
    std::vector<uint8_t> endpoint_info;
    std::string service_id;
    std::string peer_name;
  };

  struct PayloadTransferUpdate {
    int64_t payload_id = 0;
    PayloadStatus status = PayloadStatus::kFailure;
    uint64_t total_bytes = 0;
    uint64_t bytes_transferred = 0;
  };

  struct Payload {
    enum class Type {
      kUnknown = 0,
      kBytes = 1,
      kFile = 3,
    };

    int64_t id = 0;
    Type type = Type::kUnknown;
    std::vector<uint8_t> bytes;
    std::string file_path;
    std::string file_name;
    std::string parent_folder;
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
  };

  struct DiscoveryOptions {
    Strategy strategy = Strategy::kP2pCluster;
    MediumSelection allowed_mediums;
  };

  struct ConnectionOptions {
    MediumSelection allowed_mediums;
    bool non_disruptive_hotspot_mode = false;
  };

  struct ConnectionListener {
    std::function<void(const std::string &, const ConnectionInfo &)>
        initiated_cb;
    std::function<void(const std::string &)> accepted_cb;
    std::function<void(const std::string &, Status)> rejected_cb;
    std::function<void(const std::string &)> disconnected_cb;
    std::function<void(const std::string &, Medium)> bandwidth_changed_cb;
  };

  struct DiscoveryListener {
    std::function<void(const std::string &, const DiscoveredEndpointInfo &)>
        endpoint_found_cb;
    std::function<void(const std::string &)> endpoint_lost_cb;
    std::function<void(const std::string &, DistanceInfo)>
        endpoint_distance_changed_cb;
  };

  struct PayloadListener {
    std::function<void(const std::string &, Payload)> payload_cb;
    std::function<void(const std::string &, const PayloadTransferUpdate &)>
        payload_progress_cb;
  };

  NearbyConnectionsQtFacade();
  ~NearbyConnectionsQtFacade();

  NearbyConnectionsQtFacade(const NearbyConnectionsQtFacade &) = delete;
  NearbyConnectionsQtFacade &
  operator=(const NearbyConnectionsQtFacade &) = delete;
  NearbyConnectionsQtFacade(NearbyConnectionsQtFacade &&) noexcept;
  NearbyConnectionsQtFacade &operator=(NearbyConnectionsQtFacade &&) noexcept;

  // Sets global Nearby flag overrides for BLE L2CAP.
  static void SetBleL2capFlagOverrides(bool enable_ble_l2cap,
                                       bool refactor_ble_l2cap);

  Payload CreateBytesPayload(std::vector<uint8_t> bytes) const;

  void StartAdvertising(const std::string &service_id,
                        const std::vector<uint8_t> &endpoint_info,
                        const AdvertisingOptions &advertising_options,
                        ConnectionListener advertising_listener,
                        std::function<void(Status)> callback);
  void StopAdvertising(const std::string &service_id,
                       std::function<void(Status)> callback);

  void StartDiscovery(const std::string &service_id,
                      const DiscoveryOptions &discovery_options,
                      DiscoveryListener discovery_listener,
                      std::function<void(Status)> callback);
  void StopDiscovery(const std::string &service_id,
                     std::function<void(Status)> callback);

  void RequestConnection(const std::string &service_id,
                         const std::vector<uint8_t> &endpoint_info,
                         const std::string &endpoint_id,
                         const ConnectionOptions &connection_options,
                         ConnectionListener connection_listener,
                         std::function<void(Status)> callback);

  void DisconnectFromEndpoint(const std::string &service_id,
                              const std::string &endpoint_id,
                              std::function<void(Status)> callback);

  void SendPayload(const std::string &service_id,
                   const std::vector<std::string> &endpoint_ids,
                   Payload payload, std::function<void(Status)> callback);
  void InitiateBandwidthUpgrade(const std::string &service_id,
                                const std::string &endpoint_id,
                                std::function<void(Status)> callback);

  void AcceptConnection(const std::string &service_id,
                        const std::string &endpoint_id,
                        PayloadListener payload_listener,
                        std::function<void(Status)> callback);

  void StopAllEndpoints(std::function<void(Status)> callback);

private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};
} // namespace sharing
} // namespace nearby

#endif // SHARING_LINUX_NEARBY_CONNECTIONS_QT_FACADE_H_
