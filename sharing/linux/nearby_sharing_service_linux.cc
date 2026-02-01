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

#include "nearby_sharing_service_linux.h"
#include "sharing/proto/enums.pb.h"

#include <algorithm>
#include <cstdint>
#include <sstream>
#include <string>
#include <utility>

#include "absl/strings/escaping.h"
#include "absl/strings/string_view.h"
#include "absl/time/time.h"
#include "connections/advertising_options.h"
#include "connections/connection_options.h"
#include "connections/discovery_options.h"
#include "connections/listeners.h"
#include "connections/medium_selector.h"
#include "connections/payload.h"
#include "connections/status.h"
#include "connections/strategy.h"
#include "internal/crypto_cros/ec_private_key.h"
#include "internal/platform/byte_array.h"
#include "internal/platform/file.h"
#include "internal/platform/logging.h"
#include "sharing/certificates/common.h"
#include <openssl/bn.h>
#include <openssl/ec.h>
#include <openssl/evp.h>

namespace nearby::sharing::linux {
namespace {
constexpr char kServiceId[] = "NearbySharing";
const connections::Strategy kStrategy = connections::Strategy::kP2pPointToPoint;
constexpr uint8_t kAdvertisementSaltSize = 2;
constexpr uint8_t kAdvertisementMetadataKeySize = 14;
constexpr uint8_t kAdvertisementVersion = 0;
constexpr uint8_t kVersionBitmask = 0b111;
constexpr uint8_t kDeviceTypeBitmask = 0b111;
constexpr uint8_t kVisibilityBitmask = 0b1;
constexpr uint8_t kTlvMinLength = 2;
constexpr uint8_t kVendorIdLength = 1;

enum class TlvTypes : uint8_t {
  kUnknown = 0,
  kQrCode = 1,
  kVendorId = 2,
};

uint8_t EncodeHeaderByte(bool has_device_name, ShareTargetType device_type) {
  uint8_t version = static_cast<uint8_t>((kAdvertisementVersion & kVersionBitmask) << 5);
  uint8_t visibility = static_cast<uint8_t>(((has_device_name ? 0 : 1) & kVisibilityBitmask) << 4);
  uint8_t type = static_cast<uint8_t>((static_cast<int32_t>(device_type) & kDeviceTypeBitmask) << 1);
  return static_cast<uint8_t>(version | visibility | type);
}

bool ShouldIncludeDeviceName(const std::optional<std::string>& device_name) {
  return device_name.has_value() && !device_name->empty();
}

TransferMetadata::Status StatusFromPayloadStatus(
    connections::PayloadProgressInfo::Status status) {
  switch (status) {
    case connections::PayloadProgressInfo::Status::kInProgress:
      return TransferMetadata::Status::kInProgress;
    case connections::PayloadProgressInfo::Status::kSuccess:
      return TransferMetadata::Status::kComplete;
    case connections::PayloadProgressInfo::Status::kFailure:
      return TransferMetadata::Status::kFailed;
    case connections::PayloadProgressInfo::Status::kCanceled:
      return TransferMetadata::Status::kCancelled;
  }
  return TransferMetadata::Status::kUnknown;
}

}  // namespace

NearbySharingServiceLinux::NearbySharingServiceLinux()
    : device_info_(::nearby::api::ImplementationPlatform::CreateDeviceInfo()),
      router_(std::make_unique<connections::ServiceControllerRouter>()),
      core_(std::make_unique<connections::Core>(router_.get())) {
  if (device_info_) {
    auto name = device_info_->GetOsDeviceName();
    if (name.has_value()) {
      device_name_override_ = *name;
    }
  }
}

NearbySharingServiceLinux::NearbySharingServiceLinux(
    std::string device_name_override)
    : device_name_override_(std::move(device_name_override)),
      device_info_(::nearby::api::ImplementationPlatform::CreateDeviceInfo()),
      router_(std::make_unique<connections::ServiceControllerRouter>()),
      core_(std::make_unique<connections::Core>(router_.get())) {}

NearbySharingServiceLinux::~NearbySharingServiceLinux() = default;

void NearbySharingServiceLinux::AddObserver(Observer* observer) {
  if (!observer) {
    return;
  }
  observers_.insert(observer);
}

void NearbySharingServiceLinux::RemoveObserver(Observer* observer) {
  observers_.erase(observer);
}

void NearbySharingServiceLinux::Shutdown(
    std::function<void(NearbySharingServiceLinux::StatusCodes)>
        status_codes_callback) {
  StopDiscovery();
  StopAdvertising();
  endpoint_to_target_.clear();
  target_id_to_endpoint_.clear();
  active_transfers_.clear();
  is_transferring_ = false;
  std::move(status_codes_callback)(StatusCodes::kOk);
}

void NearbySharingServiceLinux::RegisterSendSurface(
    TransferUpdateCallback* transfer_callback,
    ShareTargetDiscoveredCallback* discovery_callback, SendSurfaceState state,
    Advertisement::BlockedVendorId blocked_vendor_id,
    bool disable_wifi_hotspot,
    std::function<void(NearbySharingServiceLinux::StatusCodes)>
        status_codes_callback) {
  static_cast<void>(blocked_vendor_id);
  if (!transfer_callback) {
    std::move(status_codes_callback)(StatusCodes::kInvalidArgument);
    return;
  }

  send_surfaces_[transfer_callback] = SendSurface{
      .discovery_callback = discovery_callback,
      .state = state,
      .disable_wifi_hotspot = disable_wifi_hotspot,
  };

  StartDiscoveryIfNeeded();
  std::move(status_codes_callback)(StatusCodes::kOk);
}

void NearbySharingServiceLinux::UnregisterSendSurface(
    TransferUpdateCallback* transfer_callback,
    std::function<void(NearbySharingServiceLinux::StatusCodes)>
        status_codes_callback) {
  if (!transfer_callback) {
    std::move(status_codes_callback)(StatusCodes::kInvalidArgument);
    return;
  }

  send_surfaces_.erase(transfer_callback);
  if (send_surfaces_.empty()) {
    StopDiscovery();
  }

  std::move(status_codes_callback)(StatusCodes::kOk);
}

void NearbySharingServiceLinux::RegisterReceiveSurface(
    TransferUpdateCallback* transfer_callback, ReceiveSurfaceState state,
    Advertisement::BlockedVendorId vendor_id,
    std::function<void(NearbySharingServiceLinux::StatusCodes)>
        status_codes_callback) {
  if (!transfer_callback) {
    std::move(status_codes_callback)(StatusCodes::kInvalidArgument);
    return;
  }

  receive_surfaces_[transfer_callback] = ReceiveSurface{
      .state = state,
      .vendor_id = vendor_id,
  };

  StartAdvertisingIfNeeded();
  std::move(status_codes_callback)(StatusCodes::kOk);
}

void NearbySharingServiceLinux::UnregisterReceiveSurface(
    TransferUpdateCallback* transfer_callback,
    std::function<void(NearbySharingServiceLinux::StatusCodes)>
        status_codes_callback) {
  if (!transfer_callback) {
    std::move(status_codes_callback)(StatusCodes::kInvalidArgument);
    return;
  }

  receive_surfaces_.erase(transfer_callback);
  if (receive_surfaces_.empty()) {
    StopAdvertising();
  } else {
    StartAdvertisingIfNeeded();
  }

  std::move(status_codes_callback)(StatusCodes::kOk);
}

void NearbySharingServiceLinux::ClearForegroundReceiveSurfaces(
    std::function<void(NearbySharingServiceLinux::StatusCodes)>
        status_codes_callback) {
  for (auto it = receive_surfaces_.begin(); it != receive_surfaces_.end();) {
    if (it->second.state == ReceiveSurfaceState::kForeground) {
      it = receive_surfaces_.erase(it);
    } else {
      ++it;
    }
  }

  if (receive_surfaces_.empty()) {
    StopAdvertising();
  } else {
    StartAdvertisingIfNeeded();
  }

  std::move(status_codes_callback)(StatusCodes::kOk);
}

bool NearbySharingServiceLinux::IsTransferring() const {
  return is_transferring_;
}

bool NearbySharingServiceLinux::IsScanning() const { return is_scanning_; }

bool NearbySharingServiceLinux::IsBluetoothPresent() const {
  return bluetooth_adapter_.IsValid();
}

bool NearbySharingServiceLinux::IsBluetoothPowered() const {
  return bluetooth_adapter_.IsValid() && bluetooth_adapter_.IsEnabled();
}

bool NearbySharingServiceLinux::IsExtendedAdvertisingSupported() const {
  return true;
}

bool NearbySharingServiceLinux::IsLanConnected() const { return false; }

std::string NearbySharingServiceLinux::GenerateQrCodeUrl() const {
  // Generate ECDSA P-256 key pair
  auto ec_key = nearby::crypto::ECPrivateKey::Create();
  if (!ec_key) {
    LOG(ERROR) << "Failed to generate EC key for QR code";
    return "";
  }

  // Get the EC_KEY from EVP_PKEY
  EC_KEY* raw_ec_key = EVP_PKEY_get0_EC_KEY(ec_key->key());
  if (!raw_ec_key) {
    LOG(ERROR) << "Failed to get EC_KEY from EVP_PKEY";
    return "";
  }

  const EC_GROUP* group = EC_KEY_get0_group(raw_ec_key);
  const EC_POINT* pub_key = EC_KEY_get0_public_key(raw_ec_key);
  if (!group || !pub_key) {
    LOG(ERROR) << "Failed to get EC group or public key";
    return "";
  }

  // Get the X and Y coordinates
  BIGNUM* x = BN_new();
  BIGNUM* y = BN_new();
  if (!x || !y) {
    BN_free(x);
    BN_free(y);
    LOG(ERROR) << "Failed to allocate BIGNUMs";
    return "";
  }

  if (!EC_POINT_get_affine_coordinates_GFp(group, pub_key, x, y, nullptr)) {
    BN_free(x);
    BN_free(y);
    LOG(ERROR) << "Failed to get affine coordinates";
    return "";
  }

  // Convert X coordinate to bytes (32 bytes for P-256)
  std::vector<uint8_t> x_bytes(32, 0);
  int x_len = BN_num_bytes(x);
  if (x_len > 32) {
    BN_free(x);
    BN_free(y);
    LOG(ERROR) << "X coordinate too large";
    return "";
  }
  
  // Pad with leading zeros if needed
  int offset = 32 - x_len;
  BN_bn2bin(x, x_bytes.data() + offset);

  // Determine prefix byte based on Y coordinate parity
  // 0x02 if Y is even, 0x03 if Y is odd
  uint8_t prefix = BN_is_odd(y) ? 0x03 : 0x02;

  BN_free(x);
  BN_free(y);

  // Build the key data: [version (2 bytes), prefix (1 byte), X coordinate (32 bytes)]
  std::vector<uint8_t> key_data;
  key_data.reserve(35);  // 2 + 1 + 32
  key_data.push_back(0x00);  // Version byte 1
  key_data.push_back(0x00);  // Version byte 2
  key_data.push_back(prefix);  // Prefix byte (0x02 or 0x03)
  key_data.insert(key_data.end(), x_bytes.begin(), x_bytes.end());  // X coordinate

  // Base64url encode
  std::string encoded;
  absl::WebSafeBase64Escape(
      std::string(reinterpret_cast<const char*>(key_data.data()), key_data.size()),
      &encoded);

  // Build the final URL
  return "https://quickshare.google/qrcode#key=" + encoded;
}

std::string NearbySharingServiceLinux::GetQrCodeUrl() const {
  // Generate the URL once and cache it
  if (qr_code_url_.empty()) {
    qr_code_url_ = GenerateQrCodeUrl();
  }
  return qr_code_url_;
}

void NearbySharingServiceLinux::SendAttachments(
    int64_t share_target_id,
    std::unique_ptr<nearby::sharing::AttachmentContainer>
        attachment_container,
    std::function<void(NearbySharingServiceLinux::StatusCodes)>
        status_codes_callback) {
  auto endpoint_id = GetEndpointIdForTarget(share_target_id);
  if (!endpoint_id.has_value() || !attachment_container ||
      !attachment_container->HasAttachments()) {
    std::move(status_codes_callback)(StatusCodes::kInvalidArgument);
    return;
  }

  TransferUpdateCallback* callback = PickSendTransferCallback();
  if (!callback) {
    std::move(status_codes_callback)(StatusCodes::kOutOfOrderApiCall);
    return;
  }

  TransferState transfer_state;
  transfer_state.attachments = *attachment_container;
  transfer_state.callback = callback;
  transfer_state.is_incoming = false;
  active_transfers_[*endpoint_id] = transfer_state;

  TransferMetadata metadata =
      TransferMetadataBuilder().set_status(TransferMetadata::Status::kConnecting)
          .set_progress(0)
          .set_total_attachments_count(
              attachment_container->GetAttachmentCount())
          .build();
  if (auto share_target = GetShareTarget(*endpoint_id)) {
    NotifyTransferUpdate(*share_target, transfer_state, metadata);
  }

  connections::ConnectionOptions options;
  options.strategy = kStrategy;
  options.allowed.SetAll(true);

  std::optional<std::string> device_name =
      device_name_override_.empty()
          ? std::optional<std::string>(std::nullopt)
          : std::optional<std::string>(device_name_override_);
  if (!device_name.has_value() && device_info_) {
    auto name = device_info_->GetOsDeviceName();
    if (name.has_value()) {
      device_name = *name;
    }
  }

  ShareTargetType device_type = ShareTargetType::kUnknown;
  if (device_info_) {
    device_type =
        static_cast<ShareTargetType>(device_info_->GetDeviceType());
  }

  std::vector<uint8_t> endpoint_info =
      BuildAdvertisement(device_name, device_type,
                         static_cast<uint8_t>(
                             Advertisement::BlockedVendorId::kNone));
  connections::ConnectionRequestInfo request_info;
  request_info.endpoint_info =
      ByteArray(std::string(endpoint_info.begin(), endpoint_info.end()));
  request_info.listener.initiated_cb =
      [this](const std::string& id,
             const connections::ConnectionResponseInfo& info) {
        HandleOutgoingConnectionInitiated(id, info);
      };
  request_info.listener.accepted_cb = [this](const std::string& id) {
    HandleConnectionAccepted(id, /*is_incoming=*/false);
  };
  request_info.listener.rejected_cb =
      [this](const std::string& id, connections::Status status) {
        HandleConnectionRejected(id, status, /*is_incoming=*/false);
      };
  request_info.listener.disconnected_cb = [this](const std::string& id) {
    HandleConnectionDisconnected(id);
  };

  core_->RequestConnection(*endpoint_id, request_info, options,
                           [this, cb = std::move(status_codes_callback)](
                               connections::Status status) mutable {
                             cb(StatusFromConnections(status));
                           });
}

void NearbySharingServiceLinux::Accept(
    int64_t share_target_id,
    std::function<void(NearbySharingServiceLinux::StatusCodes status_codes)>
        status_codes_callback) {
  auto endpoint_id = GetEndpointIdForTarget(share_target_id);
  if (!endpoint_id.has_value()) {
    std::move(status_codes_callback)(StatusCodes::kInvalidArgument);
    return;
  }

  core_->AcceptConnection(*endpoint_id, MakePayloadListener(true),
                          [this, cb = std::move(status_codes_callback)](
                              connections::Status status) mutable {
                            cb(StatusFromConnections(status));
                          });
}

void NearbySharingServiceLinux::Reject(
    int64_t share_target_id,
    std::function<void(NearbySharingServiceLinux::StatusCodes status_codes)>
        status_codes_callback) {
  auto endpoint_id = GetEndpointIdForTarget(share_target_id);
  if (!endpoint_id.has_value()) {
    std::move(status_codes_callback)(StatusCodes::kInvalidArgument);
    return;
  }

  core_->RejectConnection(*endpoint_id,
                          [this, cb = std::move(status_codes_callback)](
                              connections::Status status) mutable {
                            cb(StatusFromConnections(status));
                          });
}

void NearbySharingServiceLinux::Cancel(
    int64_t share_target_id,
    std::function<void(NearbySharingServiceLinux::StatusCodes status_codes)>
        status_codes_callback) {
  auto endpoint_id = GetEndpointIdForTarget(share_target_id);
  if (!endpoint_id.has_value()) {
    std::move(status_codes_callback)(StatusCodes::kInvalidArgument);
    return;
  }

  core_->DisconnectFromEndpoint(
      *endpoint_id,
      [this, cb = std::move(status_codes_callback)](connections::Status status) mutable {
        cb(StatusFromConnections(status));
      });
}

void NearbySharingServiceLinux::SetVisibility(
    proto::DeviceVisibility visibility, absl::Duration expiration,
    absl::AnyInvocable<void(StatusCodes status_code) &&> callback) {
  static_cast<void>(visibility);
  static_cast<void>(expiration);
  std::move(callback)(StatusCodes::kOk);
}

std::string NearbySharingServiceLinux::Dump() const {
  std::stringstream ss;
  ss << "NearbySharingServiceLinux";
  ss << " advertising=" << (is_advertising_ ? "true" : "false");
  ss << " scanning=" << (is_scanning_ ? "true" : "false");
  ss << " transfers=" << active_transfers_.size();
  ss << " targets=" << endpoint_to_target_.size();
  return ss.str();
}

void NearbySharingServiceLinux::UpdateFilePathsInProgress(
    bool update_file_paths) {}

NearbyShareSettings* NearbySharingServiceLinux::GetSettings() {
  return nullptr;
}

NearbyShareLocalDeviceDataManager*
NearbySharingServiceLinux::GetLocalDeviceDataManager() {
  return nullptr;
}

NearbyShareContactManager* NearbySharingServiceLinux::GetContactManager() {
  return nullptr;
}

NearbyShareCertificateManager*
NearbySharingServiceLinux::GetCertificateManager() {
  return nullptr;
}

AccountManager* NearbySharingServiceLinux::GetAccountManager() {
  return nullptr;
}

Clock& NearbySharingServiceLinux::GetClock() { return clock_; }

void NearbySharingServiceLinux::SetAlternateServiceUuidForDiscovery(
    uint16_t alternate_service_uuid) {
  alternate_service_uuid_ = alternate_service_uuid;
  if (is_scanning_) {
    StopDiscovery();
    StartDiscoveryIfNeeded();
  }
}

void NearbySharingServiceLinux::StartAdvertisingIfNeeded() {
  if (receive_surfaces_.empty()) {
    StopAdvertising();
    return;
  }

  bool has_foreground = false;
  uint8_t vendor_id = 0;
  for (const auto& [callback, surface] : receive_surfaces_) {
    if (surface.state == ReceiveSurfaceState::kForeground) {
      has_foreground = true;
      vendor_id = static_cast<uint8_t>(surface.vendor_id);
      break;
    }
  }

  std::optional<std::string> device_name = std::nullopt;
  if (has_foreground) {
    if (!device_name_override_.empty()) {
      device_name = device_name_override_;
    } else if (device_info_) {
      auto name = device_info_->GetOsDeviceName();
      if (name.has_value()) {
        device_name = *name;
      }
    }
  }

  ShareTargetType device_type = ShareTargetType::kUnknown;
  if (device_info_) {
    device_type =
        static_cast<ShareTargetType>(device_info_->GetDeviceType());
  }

  if (is_advertising_ && has_foreground == last_advertise_with_name_ &&
      vendor_id == last_advertise_vendor_id_) {
    return;
  }

  if (is_advertising_) {
    StopAdvertising();
  }

  std::vector<uint8_t> endpoint_info =
      BuildAdvertisement(device_name, device_type, vendor_id);

  connections::AdvertisingOptions options;
  options.strategy = kStrategy;
  options.allowed.SetAll(true);
  options.use_stable_endpoint_id = has_foreground;

  connections::ConnectionRequestInfo request_info;
  request_info.endpoint_info =
      ByteArray(std::string(endpoint_info.begin(), endpoint_info.end()));
  request_info.listener.initiated_cb =
      [this](const std::string& id,
             const connections::ConnectionResponseInfo& info) {
        HandleIncomingConnectionInitiated(id, info);
      };
  request_info.listener.accepted_cb = [this](const std::string& id) {
    HandleConnectionAccepted(id, /*is_incoming=*/true);
  };
  request_info.listener.rejected_cb =
      [this](const std::string& id, connections::Status status) {
        HandleConnectionRejected(id, status, /*is_incoming=*/true);
      };
  request_info.listener.disconnected_cb = [this](const std::string& id) {
    HandleConnectionDisconnected(id);
  };

  core_->StartAdvertising(
      kServiceId, options, std::move(request_info),
      [this, has_foreground, vendor_id](connections::Status status) {
        is_advertising_ = status.Ok();
        if (is_advertising_) {
          last_advertise_with_name_ = has_foreground;
          last_advertise_vendor_id_ = vendor_id;
        }
      });
}

void NearbySharingServiceLinux::StopAdvertising() {
  if (!is_advertising_) {
    return;
  }
  is_advertising_ = false;
  core_->StopAdvertising([this](connections::Status status) {
    static_cast<void>(status);
  });
}

void NearbySharingServiceLinux::StartDiscoveryIfNeeded() {
  bool needs_scanning = false;
  for (const auto& [callback, surface] : send_surfaces_) {
    if (surface.state == SendSurfaceState::kForeground) {
      needs_scanning = true;
      break;
    }
  }

  if (!needs_scanning) {
    StopDiscovery();
    return;
  }

  if (is_scanning_) {
    return;
  }

  connections::DiscoveryOptions options;
  options.strategy = kStrategy;
  options.allowed.SetAll(true);
  if (alternate_service_uuid_.has_value()) {
    options.ble_options.alternate_uuid = *alternate_service_uuid_;
  }

  connections::DiscoveryListener listener;
  listener.endpoint_found_cb =
      [this](const std::string& endpoint_id, const ByteArray& endpoint_info,
             const std::string& service_id) {
        static_cast<void>(service_id);
        std::string info_string = std::string(endpoint_info);
        std::vector<uint8_t> info_bytes(info_string.begin(), info_string.end());
        ParsedAdvertisement parsed;
        if (auto parsed_opt = ParseAdvertisement(info_bytes)) {
          parsed = *parsed_opt;
        }

        ShareTarget target;
        target.id = next_share_target_id_++;
        if (parsed.device_name.has_value()) {
          target.device_name = *parsed.device_name;
        } else {
          target.device_name = endpoint_id;
        }
        target.type = parsed.device_type;
        target.is_incoming = false;
        target.vendor_id = parsed.vendor_id;

        auto existing = endpoint_to_target_.find(endpoint_id);
        if (existing == endpoint_to_target_.end()) {
          endpoint_to_target_[endpoint_id] = target;
          target_id_to_endpoint_[target.id] = endpoint_id;
          NotifyShareTargetDiscovered(target);
        } else {
          target.id = existing->second.id;
          endpoint_to_target_[endpoint_id] = target;
          NotifyShareTargetUpdated(target);
        }
      };
  listener.endpoint_lost_cb = [this](const std::string& endpoint_id) {
    auto it = endpoint_to_target_.find(endpoint_id);
    if (it == endpoint_to_target_.end()) {
      return;
    }
    ShareTarget target = it->second;
    endpoint_to_target_.erase(it);
    target_id_to_endpoint_.erase(target.id);
    NotifyShareTargetLost(target);
  };

  core_->StartDiscovery(
      kServiceId, options, std::move(listener),
      [this](connections::Status status) { is_scanning_ = status.Ok(); });
}

void NearbySharingServiceLinux::StopDiscovery() {
  if (!is_scanning_) {
    return;
  }
  core_->StopDiscovery([this](connections::Status status) {
    if (status.Ok()) {
      is_scanning_ = false;
    }
  });
}

std::vector<uint8_t> NearbySharingServiceLinux::BuildAdvertisement(
    const std::optional<std::string>& device_name, ShareTargetType device_type,
    uint8_t vendor_id) const {
  const bool has_device_name = ShouldIncludeDeviceName(device_name);
  std::vector<uint8_t> salt = GenerateRandomBytes(kAdvertisementSaltSize);
  std::vector<uint8_t> metadata_key =
      GenerateRandomBytes(kAdvertisementMetadataKeySize);

  size_t size = 1 + salt.size() + metadata_key.size();
  if (has_device_name) {
    size += 1 + device_name->size();
  }
  if (vendor_id != 0) {
    size += kTlvMinLength + kVendorIdLength;
  }

  std::vector<uint8_t> endpoint_info;
  endpoint_info.reserve(size);
  endpoint_info.push_back(EncodeHeaderByte(has_device_name, device_type));
  endpoint_info.insert(endpoint_info.end(), salt.begin(), salt.end());
  endpoint_info.insert(endpoint_info.end(), metadata_key.begin(),
                       metadata_key.end());

  if (has_device_name) {
    endpoint_info.push_back(
        static_cast<uint8_t>(device_name->size() & 0xff));
    endpoint_info.insert(endpoint_info.end(), device_name->begin(),
                         device_name->end());
  }

  if (vendor_id != 0) {
    endpoint_info.push_back(static_cast<uint8_t>(TlvTypes::kVendorId));
    endpoint_info.push_back(kVendorIdLength);
    endpoint_info.push_back(vendor_id);
  }

  return endpoint_info;
}

std::optional<NearbySharingServiceLinux::ParsedAdvertisement>
NearbySharingServiceLinux::ParseAdvertisement(
    absl::Span<const uint8_t> endpoint_info) const {
  ParsedAdvertisement parsed;
  const size_t minimum_size =
      1 + kAdvertisementSaltSize + kAdvertisementMetadataKeySize;
  if (endpoint_info.size() < minimum_size) {
    return std::nullopt;
  }

  size_t offset = 0;
  uint8_t header = endpoint_info[offset++];
  bool has_device_name = ((header >> 4) & kVisibilityBitmask) == 0;
  uint8_t type = (header >> 1) & kDeviceTypeBitmask;
  if (type <= static_cast<uint8_t>(ShareTargetType::kXR)) {
    parsed.device_type = static_cast<ShareTargetType>(type);
  } else {
    parsed.device_type = ShareTargetType::kUnknown;
  }

  offset += kAdvertisementSaltSize + kAdvertisementMetadataKeySize;
  if (has_device_name) {
    if (offset >= endpoint_info.size()) {
      return parsed;
    }
    uint8_t name_length = endpoint_info[offset++];
    if (name_length == 0 || offset + name_length > endpoint_info.size()) {
      return parsed;
    }
    parsed.device_name = std::string(
        reinterpret_cast<const char*>(endpoint_info.data() + offset),
        name_length);
    offset += name_length;
  }

  while (offset + kTlvMinLength <= endpoint_info.size()) {
    uint8_t tlv_type = endpoint_info[offset++];
    uint8_t tlv_length = endpoint_info[offset++];
    if (offset + tlv_length > endpoint_info.size()) {
      break;
    }
    if (tlv_type == static_cast<uint8_t>(TlvTypes::kVendorId) &&
        tlv_length == kVendorIdLength) {
      parsed.vendor_id = endpoint_info[offset];
    }
    offset += tlv_length;
  }

  return parsed;
}

void NearbySharingServiceLinux::NotifyShareTargetDiscovered(
    const ShareTarget& share_target) {
  for (const auto& [transfer_callback, surface] : send_surfaces_) {
    if (surface.state != SendSurfaceState::kForeground ||
        surface.discovery_callback == nullptr) {
      continue;
    }
    surface.discovery_callback->OnShareTargetDiscovered(share_target);
  }
}

void NearbySharingServiceLinux::NotifyShareTargetUpdated(
    const ShareTarget& share_target) {
  for (const auto& [transfer_callback, surface] : send_surfaces_) {
    if (surface.state != SendSurfaceState::kForeground ||
        surface.discovery_callback == nullptr) {
      continue;
    }
    surface.discovery_callback->OnShareTargetUpdated(share_target);
  }
}

void NearbySharingServiceLinux::NotifyShareTargetLost(
    const ShareTarget& share_target) {
  for (const auto& [transfer_callback, surface] : send_surfaces_) {
    if (surface.state != SendSurfaceState::kForeground ||
        surface.discovery_callback == nullptr) {
      continue;
    }
    surface.discovery_callback->OnShareTargetLost(share_target);
  }
}

void NearbySharingServiceLinux::NotifyTransferUpdate(
    const ShareTarget& share_target, const TransferState& transfer_state,
    const TransferMetadata& metadata) {
  if (!transfer_state.callback) {
    return;
  }
  transfer_state.callback->OnTransferUpdate(share_target,
                                            transfer_state.attachments,
                                            metadata);
}

TransferUpdateCallback* NearbySharingServiceLinux::PickSendTransferCallback()
    const {
  if (send_surfaces_.empty()) {
    return nullptr;
  }
  return send_surfaces_.begin()->first;
}

TransferUpdateCallback* NearbySharingServiceLinux::PickReceiveTransferCallback()
    const {
  if (receive_surfaces_.empty()) {
    return nullptr;
  }
  return receive_surfaces_.begin()->first;
}

std::optional<std::string> NearbySharingServiceLinux::GetEndpointIdForTarget(
    int64_t share_target_id) const {
  auto it = target_id_to_endpoint_.find(share_target_id);
  if (it == target_id_to_endpoint_.end()) {
    return std::nullopt;
  }
  return it->second;
}

std::optional<ShareTarget> NearbySharingServiceLinux::GetShareTarget(
    absl::string_view endpoint_id) const {
  auto it = endpoint_to_target_.find(std::string(endpoint_id));
  if (it == endpoint_to_target_.end()) {
    return std::nullopt;
  }
  return it->second;
}

void NearbySharingServiceLinux::HandleIncomingConnectionInitiated(
    const std::string& endpoint_id,
    const connections::ConnectionResponseInfo& info) {
  static_cast<void>(info);
  std::vector<uint8_t> info_bytes(info.remote_endpoint_info.begin(),
                                  info.remote_endpoint_info.end());
  ParsedAdvertisement parsed;
  if (auto parsed_opt = ParseAdvertisement(info_bytes)) {
    parsed = *parsed_opt;
  }

  ShareTarget target;
  target.id = next_share_target_id_++;
  target.device_name = parsed.device_name.value_or(endpoint_id);
  target.type = parsed.device_type;
  target.is_incoming = true;
  target.vendor_id = parsed.vendor_id;

  auto existing = endpoint_to_target_.find(endpoint_id);
  if (existing == endpoint_to_target_.end()) {
    endpoint_to_target_[endpoint_id] = target;
    target_id_to_endpoint_[target.id] = endpoint_id;
  } else {
    target.id = existing->second.id;
    endpoint_to_target_[endpoint_id] = target;
  }

  TransferState transfer_state;
  transfer_state.attachments = AttachmentContainer();
  transfer_state.callback = PickReceiveTransferCallback();
  transfer_state.is_incoming = true;
  active_transfers_[endpoint_id] = transfer_state;

  TransferMetadata metadata = TransferMetadataBuilder()
                                  .set_status(TransferMetadata::Status::kAwaitingLocalConfirmation)
                                  .set_progress(0)
                                  .build();
  NotifyTransferUpdate(target, transfer_state, metadata);
}

void NearbySharingServiceLinux::HandleOutgoingConnectionInitiated(
    const std::string& endpoint_id,
    const connections::ConnectionResponseInfo& info) {
  static_cast<void>(info);
  core_->AcceptConnection(endpoint_id, MakePayloadListener(false),
                          [this, endpoint_id](connections::Status status) {
                            if (!status.Ok()) {
                              HandleConnectionRejected(endpoint_id, status,
                                                       /*is_incoming=*/false);
                            }
                          });

  auto share_target = GetShareTarget(endpoint_id);
  auto transfer_it = active_transfers_.find(endpoint_id);
  if (share_target && transfer_it != active_transfers_.end()) {
    TransferMetadata metadata =
        TransferMetadataBuilder()
            .set_status(TransferMetadata::Status::kAwaitingRemoteAcceptance)
            .set_progress(0)
            .build();
    NotifyTransferUpdate(*share_target, transfer_it->second, metadata);
  }
}

void NearbySharingServiceLinux::HandleConnectionAccepted(
    const std::string& endpoint_id, bool is_incoming) {
  auto transfer_it = active_transfers_.find(endpoint_id);
  if (transfer_it == active_transfers_.end()) {
    return;
  }

  auto share_target = GetShareTarget(endpoint_id);
  if (share_target) {
    TransferMetadata metadata = TransferMetadataBuilder()
                                    .set_status(TransferMetadata::Status::kInProgress)
                                    .set_progress(0)
                                    .set_total_attachments_count(
                                        transfer_it->second.attachments.GetAttachmentCount())
                                    .build();
    NotifyTransferUpdate(*share_target, transfer_it->second, metadata);
  }

  if (!is_incoming) {
    const AttachmentContainer& attachments = transfer_it->second.attachments;
    std::unique_ptr<connections::Payload> payload;
    if (!attachments.GetTextAttachments().empty()) {
      std::string text =
          std::string(attachments.GetTextAttachments()[0].text_body());
      payload = std::make_unique<connections::Payload>(ByteArray(text));
    } else if (!attachments.GetFileAttachments().empty()) {
      const auto& file_attachment = attachments.GetFileAttachments()[0];
      if (file_attachment.file_path().has_value()) {
        std::string file_path = file_attachment.file_path()->ToString();
        nearby::InputFile input_file(file_path, file_attachment.size());
        payload = std::make_unique<connections::Payload>(
            std::string(file_attachment.parent_folder()),
            std::string(file_attachment.file_name()), std::move(input_file));
      }
    }

    if (payload) {
      std::vector<std::string> endpoints;
      endpoints.push_back(endpoint_id);
      core_->SendPayload(
          endpoints, std::move(*payload),
          [this](connections::Status status) {
            if (!status.Ok()) {
              is_transferring_ = false;
            }
          });
    }
  }

  is_transferring_ = true;
}

void NearbySharingServiceLinux::HandleConnectionRejected(
    const std::string& endpoint_id, connections::Status status,
    bool is_incoming) {
  static_cast<void>(status);
  static_cast<void>(is_incoming);
  auto transfer_it = active_transfers_.find(endpoint_id);
  if (transfer_it == active_transfers_.end()) {
    return;
  }
  auto share_target = GetShareTarget(endpoint_id);
  if (share_target) {
    TransferMetadata metadata = TransferMetadataBuilder()
                                    .set_status(TransferMetadata::Status::kRejected)
                                    .set_progress(0)
                                    .build();
    NotifyTransferUpdate(*share_target, transfer_it->second, metadata);
  }
  active_transfers_.erase(transfer_it);
  is_transferring_ = false;
}

void NearbySharingServiceLinux::HandleConnectionDisconnected(
    const std::string& endpoint_id) {
  active_transfers_.erase(endpoint_id);
  if (active_transfers_.empty()) {
    is_transferring_ = false;
  }
}

connections::PayloadListener NearbySharingServiceLinux::MakePayloadListener(
    bool is_incoming) {
  static_cast<void>(is_incoming);
  connections::PayloadListener listener;
  listener.payload_cb =
      [this, is_incoming](absl::string_view endpoint_id,
                          connections::Payload payload) {
        auto transfer_it = active_transfers_.find(std::string(endpoint_id));
        if (transfer_it == active_transfers_.end()) {
          return;
        }
        auto share_target = GetShareTarget(endpoint_id);
        if (!share_target) {
          return;
        }

        TransferMetadata metadata = TransferMetadataBuilder()
                                        .set_status(TransferMetadata::Status::kInProgress)
                                        .set_progress(0)
                                        .build();
        NotifyTransferUpdate(*share_target, transfer_it->second, metadata);
      };

  listener.payload_progress_cb =
      [this, is_incoming](absl::string_view endpoint_id,
                          const connections::PayloadProgressInfo& info) {
        auto transfer_it = active_transfers_.find(std::string(endpoint_id));
        if (transfer_it == active_transfers_.end()) {
          return;
        }
        auto share_target = GetShareTarget(endpoint_id);
        if (!share_target) {
          return;
        }

        float progress = 0.0f;
        if (info.total_bytes > 0) {
          progress = static_cast<float>(info.bytes_transferred) /
                     static_cast<float>(info.total_bytes);
        }

        TransferMetadata metadata =
            TransferMetadataBuilder()
                .set_status(StatusFromPayloadStatus(info.status))
                .set_progress(progress)
                .set_transferred_bytes(info.bytes_transferred)
                .build();

        NotifyTransferUpdate(*share_target, transfer_it->second, metadata);

        if (TransferMetadata::IsFinalStatus(metadata.status())) {
          active_transfers_.erase(transfer_it);
          if (active_transfers_.empty()) {
            is_transferring_ = false;
          }
        }
      };
  return listener;
}

NearbySharingService::StatusCodes NearbySharingServiceLinux::StatusFromConnections(
    connections::Status status) const {
  if (status.Ok()) {
    return StatusCodes::kOk;
  }
  if (status.value == connections::Status::kOutOfOrderApiCall) {
    return StatusCodes::kOutOfOrderApiCall;
  }
  return StatusCodes::kError;
}

}  // namespace nearby::sharing::linux
