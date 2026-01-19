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

#ifndef THIRD_PARTY_NEARBY_SHARING_LINUX_NEARBY_SHARING_SERVICE_LINUX_H_
#define THIRD_PARTY_NEARBY_SHARING_LINUX_NEARBY_SHARING_SERVICE_LINUX_H_

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "absl/functional/any_invocable.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "connections/advertising_options.h"
#include "connections/connection_options.h"
#include "connections/core.h"
#include "connections/discovery_options.h"
#include "connections/implementation/service_controller_router.h"
#include "connections/listeners.h"
#include "connections/payload.h"
#include "connections/status.h"
#include "connections/strategy.h"
#include "internal/platform/bluetooth_adapter.h"
#include "internal/platform/clock_impl.h"
#include "internal/platform/implementation/platform.h"
#include "sharing/attachment_container.h"
#include "sharing/nearby_sharing_service.h"
#include "sharing/share_target.h"
#include "sharing/share_target_discovered_callback.h"
#include "sharing/transfer_metadata.h"
#include "sharing/transfer_metadata_builder.h"

namespace nearby::sharing::linux {

class NearbySharingServiceLinux : public NearbySharingService {
 public:
  using StatusCodes = NearbySharingService::StatusCodes;

  NearbySharingServiceLinux();
  explicit NearbySharingServiceLinux(std::string device_name_override);
  ~NearbySharingServiceLinux() override;

  void AddObserver(Observer* observer) override;
  void RemoveObserver(Observer* observer) override;

  void Shutdown(
      std::function<void(StatusCodes)> status_codes_callback) override;

  void RegisterSendSurface(
      TransferUpdateCallback* transfer_callback,
      ShareTargetDiscoveredCallback* discovery_callback, SendSurfaceState state,
      Advertisement::BlockedVendorId blocked_vendor_id,
      bool disable_wifi_hotspot,
      std::function<void(StatusCodes)> status_codes_callback) override;

  void UnregisterSendSurface(
      TransferUpdateCallback* transfer_callback,
      std::function<void(StatusCodes)> status_codes_callback) override;

  void RegisterReceiveSurface(
      TransferUpdateCallback* transfer_callback, ReceiveSurfaceState state,
      Advertisement::BlockedVendorId vendor_id,
      std::function<void(StatusCodes)> status_codes_callback) override;

  void UnregisterReceiveSurface(
      TransferUpdateCallback* transfer_callback,
      std::function<void(StatusCodes)> status_codes_callback) override;

  void ClearForegroundReceiveSurfaces(
      std::function<void(StatusCodes)> status_codes_callback) override;

  bool IsTransferring() const override;
  bool IsScanning() const override;
  bool IsBluetoothPresent() const override;
  bool IsBluetoothPowered() const override;
  bool IsExtendedAdvertisingSupported() const override;
  bool IsLanConnected() const override;
  std::string GetQrCodeUrl() const override;

  void SendAttachments(
      int64_t share_target_id,
      std::unique_ptr<nearby::sharing::AttachmentContainer>
          attachment_container,
      std::function<void(StatusCodes)> status_codes_callback) override;

  void Accept(int64_t share_target_id,
              std::function<void(StatusCodes status_codes)>
                  status_codes_callback) override;

  void Reject(int64_t share_target_id,
              std::function<void(StatusCodes status_codes)>
                  status_codes_callback) override;

  void Cancel(int64_t share_target_id,
              std::function<void(StatusCodes status_codes)>
                  status_codes_callback) override;

  void SetVisibility(proto::DeviceVisibility visibility,
                     absl::Duration expiration,
                     absl::AnyInvocable<void(StatusCodes status_code) &&>
                         callback) override;

  std::string Dump() const override;
  void UpdateFilePathsInProgress(bool update_file_paths) override;

  NearbyShareSettings* GetSettings() override;
  NearbyShareLocalDeviceDataManager* GetLocalDeviceDataManager() override;
  NearbyShareContactManager* GetContactManager() override;
  NearbyShareCertificateManager* GetCertificateManager() override;
  AccountManager* GetAccountManager() override;
  Clock& GetClock() override;
  void SetAlternateServiceUuidForDiscovery(
      uint16_t alternate_service_uuid) override;

 private:
  struct SendSurface {
    ShareTargetDiscoveredCallback* discovery_callback = nullptr;
    SendSurfaceState state = SendSurfaceState::kUnknown;
    bool disable_wifi_hotspot = false;
  };

  struct ReceiveSurface {
    ReceiveSurfaceState state = ReceiveSurfaceState::kUnknown;
    Advertisement::BlockedVendorId vendor_id =
        Advertisement::BlockedVendorId::kNone;
  };

  struct TransferState {
    nearby::sharing::AttachmentContainer attachments;
    TransferUpdateCallback* callback = nullptr;
    bool is_incoming = false;
  };

  struct ParsedAdvertisement {
    ShareTargetType device_type = ShareTargetType::kUnknown;
    std::optional<std::string> device_name;
    uint8_t vendor_id = 0;
  };

  void StartAdvertisingIfNeeded();
  void StopAdvertising();
  void StartDiscoveryIfNeeded();
  void StopDiscovery();

  std::vector<uint8_t> BuildAdvertisement(
      const std::optional<std::string>& device_name,
      ShareTargetType device_type, uint8_t vendor_id) const;

  std::optional<ParsedAdvertisement> ParseAdvertisement(
      absl::Span<const uint8_t> endpoint_info) const;

  void NotifyShareTargetDiscovered(const ShareTarget& share_target);
  void NotifyShareTargetUpdated(const ShareTarget& share_target);
  void NotifyShareTargetLost(const ShareTarget& share_target);
  void NotifyTransferUpdate(const ShareTarget& share_target,
                            const TransferState& transfer_state,
                            const TransferMetadata& metadata);

  TransferUpdateCallback* PickSendTransferCallback() const;
  TransferUpdateCallback* PickReceiveTransferCallback() const;

  std::optional<std::string> GetEndpointIdForTarget(
      int64_t share_target_id) const;

  std::optional<ShareTarget> GetShareTarget(
      absl::string_view endpoint_id) const;

  void HandleIncomingConnectionInitiated(
      const std::string& endpoint_id,
      const connections::ConnectionResponseInfo& info);

  void HandleOutgoingConnectionInitiated(
      const std::string& endpoint_id,
      const connections::ConnectionResponseInfo& info);

  void HandleConnectionAccepted(const std::string& endpoint_id,
                                bool is_incoming);
  void HandleConnectionRejected(const std::string& endpoint_id,
                                connections::Status status, bool is_incoming);
  void HandleConnectionDisconnected(const std::string& endpoint_id);

  connections::PayloadListener MakePayloadListener(bool is_incoming);

  StatusCodes StatusFromConnections(connections::Status status) const;

  std::string device_name_override_;
  std::unique_ptr<::nearby::api::DeviceInfo> device_info_;
  BluetoothAdapter bluetooth_adapter_;
  ClockImpl clock_;

  std::unique_ptr<connections::ServiceControllerRouter> router_;
  std::unique_ptr<connections::Core> core_;

  std::unordered_set<Observer*> observers_;
  std::unordered_map<TransferUpdateCallback*, SendSurface> send_surfaces_;
  std::unordered_map<TransferUpdateCallback*, ReceiveSurface> receive_surfaces_;

  std::unordered_map<std::string, ShareTarget> endpoint_to_target_;
  std::unordered_map<int64_t, std::string> target_id_to_endpoint_;
  std::unordered_map<std::string, TransferState> active_transfers_;

  std::optional<uint16_t> alternate_service_uuid_;
  bool is_scanning_ = false;
  bool is_advertising_ = false;
  bool is_transferring_ = false;
  int64_t next_share_target_id_ = 1;
  bool last_advertise_with_name_ = false;
  uint8_t last_advertise_vendor_id_ = 0;
};

}  // namespace nearby::sharing::linux

#endif  // THIRD_PARTY_NEARBY_SHARING_LINUX_NEARBY_SHARING_SERVICE_LINUX_H_
