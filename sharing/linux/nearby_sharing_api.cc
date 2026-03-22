#include "sharing/linux/nearby_sharing_api.h"

#include <limits>
#include <mutex>
#include <optional>
#include <string>
#include <utility>

#include <openssl/bn.h>
#include <openssl/ec.h>
#include <openssl/evp.h>

#include "absl/strings/escaping.h"
#include "absl/time/time.h"
#include "connections/implementation/flags/nearby_connections_feature_flags.h"
#include "internal/base/file_path.h"
#include "internal/base/files.h"
#include "internal/crypto_cros/ec_private_key.h"
#include "internal/flags/nearby_flags.h"
#include "sharing/analytics/analytics_recorder.h"
#include "sharing/attachment_container.h"
#include "sharing/file_attachment.h"
#include "sharing/flags/generated/nearby_sharing_feature_flags.h"
#include "sharing/linux/platform/linux_sharing_platform.h"
#include "sharing/local_device_data/nearby_share_local_device_data_manager.h"
#include "sharing/nearby_sharing_service_factory.h"
#include "sharing/proto/enums.pb.h"
#include "sharing/share_target_discovered_callback.h"
#include "sharing/transfer_metadata.h"
#include "sharing/transfer_update_callback.h"

namespace nearby::sharing {

namespace {

using NativeService = nearby::sharing::NearbySharingService;

void EnableBleL2capDefaults() {
  nearby::NearbyFlags::GetInstance().OverrideBoolFlagValue(
      nearby::connections::config_package_nearby::nearby_connections_feature::
          kEnableBleL2cap,
      true);
  nearby::NearbyFlags::GetInstance().OverrideBoolFlagValue(
      nearby::sharing::config_package_nearby::nearby_sharing_feature::
          kEnableBleForTransfer,
      true);
  //nearby::NearbyFlags::GetInstance().OverrideBoolFlagValue(
  //    nearby::connections::config_package_nearby::nearby_connections_feature::
  //        kRefactorBleL2cap,
  //    false);
}

NearbySharingApi::StatusCode ToFacadeStatus(
    nearby::sharing::NearbySharingService::StatusCodes status) {
  switch (status) {
    case nearby::sharing::NearbySharingService::StatusCodes::kOk:
      return NearbySharingApi::StatusCode::kOk;
    case nearby::sharing::NearbySharingService::StatusCodes::kError:
      return NearbySharingApi::StatusCode::kError;
    case nearby::sharing::NearbySharingService::StatusCodes::kOutOfOrderApiCall:
      return NearbySharingApi::StatusCode::kOutOfOrderApiCall;
    case nearby::sharing::NearbySharingService::StatusCodes::kStatusAlreadyStopped:
      return NearbySharingApi::StatusCode::kStatusAlreadyStopped;
    case nearby::sharing::NearbySharingService::StatusCodes::kTransferAlreadyInProgress:
      return NearbySharingApi::StatusCode::kTransferAlreadyInProgress;
    case nearby::sharing::NearbySharingService::StatusCodes::kNoAvailableConnectionMedium:
      return NearbySharingApi::StatusCode::kNoAvailableConnectionMedium;
    case nearby::sharing::NearbySharingService::StatusCodes::kIrrecoverableHardwareError:
      return NearbySharingApi::StatusCode::kIrrecoverableHardwareError;
    case nearby::sharing::NearbySharingService::StatusCodes::kInvalidArgument:
      return NearbySharingApi::StatusCode::kInvalidArgument;
  }
  return NearbySharingApi::StatusCode::kError;
}

NearbySharingApi::TransferStatus ToFacadeTransferStatus(
    nearby::sharing::TransferMetadata::Status status) {
  using NativeStatus = nearby::sharing::TransferMetadata::Status;
  using FacadeStatus = NearbySharingApi::TransferStatus;
  switch (status) {
    case NativeStatus::kUnknown:
      return FacadeStatus::kUnknown;
    case NativeStatus::kConnecting:
      return FacadeStatus::kConnecting;
    case NativeStatus::kAwaitingLocalConfirmation:
      return FacadeStatus::kAwaitingLocalConfirmation;
    case NativeStatus::kAwaitingRemoteAcceptance:
      return FacadeStatus::kAwaitingRemoteAcceptance;
    case NativeStatus::kInProgress:
      return FacadeStatus::kInProgress;
    case NativeStatus::kComplete:
      return FacadeStatus::kComplete;
    case NativeStatus::kFailed:
      return FacadeStatus::kFailed;
    case NativeStatus::kRejected:
      return FacadeStatus::kRejected;
    case NativeStatus::kCancelled:
      return FacadeStatus::kCancelled;
    case NativeStatus::kTimedOut:
      return FacadeStatus::kTimedOut;
    case NativeStatus::kMediaUnavailable:
      return FacadeStatus::kMediaUnavailable;
    case NativeStatus::kNotEnoughSpace:
      return FacadeStatus::kNotEnoughSpace;
    case NativeStatus::kUnsupportedAttachmentType:
      return FacadeStatus::kUnsupportedAttachmentType;
    case NativeStatus::kDeviceAuthenticationFailed:
      return FacadeStatus::kDeviceAuthenticationFailed;
    case NativeStatus::kIncompletePayloads:
      return FacadeStatus::kIncompletePayloads;
  }
  return FacadeStatus::kUnknown;
}

NearbySharingApi::TextAttachmentType ToFacadeTextAttachmentType(
    nearby::sharing::TextAttachment::Type type) {
  using FacadeType = NearbySharingApi::TextAttachmentType;
  switch (type) {
    case nearby::sharing::service::proto::TextMetadata::TEXT:
      return FacadeType::kText;
    case nearby::sharing::service::proto::TextMetadata::URL:
      return FacadeType::kUrl;
    case nearby::sharing::service::proto::TextMetadata::PHONE_NUMBER:
      return FacadeType::kPhoneNumber;
    case nearby::sharing::service::proto::TextMetadata::ADDRESS:
      return FacadeType::kAddress;
    case nearby::sharing::service::proto::TextMetadata::UNKNOWN:
      return FacadeType::kUnknown;
  }
  return FacadeType::kUnknown;
}

float NormalizeFacadeProgress(float progress) {
  if (progress <= 0.0f) {
    return 0.0f;
  }
  if (progress >= 100.0f) {
    return 1.0f;
  }
  return progress / 100.0f;
}

std::string GenerateQrCodeUrl() {
  auto ec_key = nearby::crypto::ECPrivateKey::Create();
  if (!ec_key) {
    return {};
  }

  const EC_KEY* raw_ec_key = EVP_PKEY_get0_EC_KEY(ec_key->key());
  if (!raw_ec_key) {
    return {};
  }

  const EC_GROUP* group = EC_KEY_get0_group(raw_ec_key);
  const EC_POINT* public_key = EC_KEY_get0_public_key(raw_ec_key);
  if (!group || !public_key) {
    return {};
  }

  BIGNUM* x = BN_new();
  BIGNUM* y = BN_new();
  if (!x || !y) {
    BN_free(x);
    BN_free(y);
    return {};
  }

  if (!EC_POINT_get_affine_coordinates_GFp(group, public_key, x, y, nullptr)) {
    BN_free(x);
    BN_free(y);
    return {};
  }

  std::vector<uint8_t> x_bytes(32, 0);
  const int x_len = BN_num_bytes(x);
  if (x_len > static_cast<int>(x_bytes.size())) {
    BN_free(x);
    BN_free(y);
    return {};
  }
  BN_bn2bin(x, x_bytes.data() + (x_bytes.size() - x_len));

  const uint8_t prefix = BN_is_odd(y) ? 0x03 : 0x02;
  BN_free(x);
  BN_free(y);

  std::vector<uint8_t> key_data;
  key_data.reserve(35);
  key_data.push_back(0x00);
  key_data.push_back(0x00);
  key_data.push_back(prefix);
  key_data.insert(key_data.end(), x_bytes.begin(), x_bytes.end());

  std::string encoded;
  absl::WebSafeBase64Escape(
      std::string(reinterpret_cast<const char*>(key_data.data()),
                  key_data.size()),
      &encoded);
  return "https://quickshare.google/qrcode#key=" + encoded;
}

}  // namespace

class NearbySharingApi::Impl : public nearby::sharing::ShareTargetDiscoveredCallback,
                               public nearby::sharing::TransferUpdateCallback {
 public:
  Impl()
      : analytics_recorder(0, nullptr),
        platform(),
        service(NearbySharingServiceFactory::GetInstance()->CreateSharingService(
            platform, &analytics_recorder, /*event_logger=*/nullptr,
            /*supports_file_sync=*/false)) {}

  explicit Impl(std::string device_name_override)
      : analytics_recorder(0, nullptr),
        device_name_override(device_name_override),
        platform(device_name_override),
        service(NearbySharingServiceFactory::GetInstance()->CreateSharingService(
            platform, &analytics_recorder, /*event_logger=*/nullptr,
            /*supports_file_sync=*/false)) {
    if (service != nullptr && !device_name_override.empty() &&
        service->GetLocalDeviceDataManager() != nullptr) {
      service->GetLocalDeviceDataManager()->SetDeviceName(device_name_override);
    }
  }

  void OnShareTargetDiscovered(const nearby::sharing::ShareTarget& share_target)
      override {
    Listener listener_copy;
    {
      std::scoped_lock lock(listener_mutex);
      listener_copy = listener;
    }
    if (!listener_copy.target_discovered_cb) {
      return;
    }
    listener_copy.target_discovered_cb(ToShareTargetInfo(share_target));
  }

  void OnShareTargetLost(const nearby::sharing::ShareTarget& share_target)
      override {
    Listener listener_copy;
    {
      std::scoped_lock lock(listener_mutex);
      listener_copy = listener;
    }
    if (!listener_copy.target_lost_cb) {
      return;
    }
    listener_copy.target_lost_cb(share_target.id);
  }

  void OnShareTargetUpdated(const nearby::sharing::ShareTarget& share_target)
      override {
    Listener listener_copy;
    {
      std::scoped_lock lock(listener_mutex);
      listener_copy = listener;
    }
    if (!listener_copy.target_updated_cb) {
      return;
    }
    listener_copy.target_updated_cb(ToShareTargetInfo(share_target));
  }

  void OnTransferUpdate(
      const nearby::sharing::ShareTarget& share_target,
      const nearby::sharing::AttachmentContainer& attachment_container,
      const nearby::sharing::TransferMetadata& transfer_metadata) override {
    Listener listener_copy;
    {
      std::scoped_lock lock(listener_mutex);
      listener_copy = listener;
    }
    if (!listener_copy.transfer_update_cb) {
      return;
    }

    NearbySharingApi::TransferUpdateInfo info;
    info.share_target_id = share_target.id;
    info.device_name = share_target.device_name;
    info.is_incoming = share_target.is_incoming;
    info.status = ToFacadeTransferStatus(transfer_metadata.status());
    info.progress = NormalizeFacadeProgress(transfer_metadata.progress());
    info.transferred_bytes = transfer_metadata.transferred_bytes();
    info.total_attachments = transfer_metadata.total_attachments_count();
    info.transferred_attachments = transfer_metadata.transferred_attachments_count();
    if (!attachment_container.GetFileAttachments().empty()) {
      const nearby::sharing::FileAttachment& file =
          attachment_container.GetFileAttachments().front();
      info.first_file_name = std::string(file.file_name());
      if (file.file_path().has_value()) {
        info.first_file_path = file.file_path()->ToString();
      }
    }
    info.text_attachments.reserve(
        attachment_container.GetTextAttachments().size());
    for (const nearby::sharing::TextAttachment& text :
         attachment_container.GetTextAttachments()) {
      NearbySharingApi::TextAttachmentInfo text_info;
      text_info.type = ToFacadeTextAttachmentType(text.type());
      text_info.text_title = std::string(text.text_title());
      text_info.text_body = std::string(text.text_body());
      info.text_attachments.push_back(std::move(text_info));
    }
    listener_copy.transfer_update_cb(info);
  }

  NearbySharingApi::ShareTargetInfo ToShareTargetInfo(
      const nearby::sharing::ShareTarget& share_target) {
    NearbySharingApi::ShareTargetInfo info;
    info.id = share_target.id;
    info.device_name = share_target.device_name;
    info.is_incoming = share_target.is_incoming;
    info.device_type = static_cast<int>(share_target.type);
    return info;
  }

  nearby::sharing::analytics::AnalyticsRecorder analytics_recorder;
  std::string device_name_override;
  LinuxSharingPlatform platform;
  NativeService* service = nullptr;
  bool send_mode_started = false;
  bool receive_mode_started = false;
  std::string qr_code_url;
  std::mutex listener_mutex;
  NearbySharingApi::Listener listener;
};

NearbySharingApi::NearbySharingApi() {
  EnableBleL2capDefaults();
  impl_ = std::make_unique<Impl>();
}

NearbySharingApi::NearbySharingApi(std::string device_name_override)
    : impl_(nullptr) {
  EnableBleL2capDefaults();
  impl_ = std::make_unique<Impl>(std::move(device_name_override));
}

NearbySharingApi::~NearbySharingApi() = default;

NearbySharingApi::NearbySharingApi(NearbySharingApi&&) noexcept = default;

NearbySharingApi& NearbySharingApi::operator=(NearbySharingApi&&) noexcept = default;

void NearbySharingApi::SetListener(Listener listener) {
  std::scoped_lock lock(impl_->listener_mutex);
  impl_->listener = std::move(listener);
}

void NearbySharingApi::StartSendMode(std::function<void(StatusCode)> callback) {
  if (impl_->service == nullptr) {
    if (callback) {
      callback(StatusCode::kError);
    }
    return;
  }
  if (impl_->send_mode_started) {
    if (callback) {
      callback(StatusCode::kOk);
    }
    return;
  }
  impl_->service->RegisterSendSurface(
      impl_.get(), impl_.get(),
      nearby::sharing::NearbySharingService::SendSurfaceState::kForeground,
      nearby::sharing::Advertisement::BlockedVendorId::kNone,
      /*disable_wifi_hotspot=*/false,
      [this, cb = std::move(callback)](
          nearby::sharing::NearbySharingService::StatusCodes status) mutable {
        if (status == nearby::sharing::NearbySharingService::StatusCodes::kOk) {
          impl_->send_mode_started = true;
        }
        if (cb) {
          cb(ToFacadeStatus(status));
        }
      });
}

void NearbySharingApi::StopSendMode(std::function<void(StatusCode)> callback) {
  if (impl_->service == nullptr) {
    if (callback) {
      callback(StatusCode::kError);
    }
    return;
  }
  if (!impl_->send_mode_started) {
    if (callback) {
      callback(StatusCode::kStatusAlreadyStopped);
    }
    return;
  }
  impl_->service->UnregisterSendSurface(
      impl_.get(),
      [this, cb = std::move(callback)](
          nearby::sharing::NearbySharingService::StatusCodes status) mutable {
        if (status == nearby::sharing::NearbySharingService::StatusCodes::kOk) {
          impl_->send_mode_started = false;
        }
        if (cb) {
          cb(ToFacadeStatus(status));
        }
      });
}

void NearbySharingApi::StartReceiveMode(std::function<void(StatusCode)> callback) {
  if (impl_->service == nullptr) {
    if (callback) {
      callback(StatusCode::kError);
    }
    return;
  }
  if (impl_->receive_mode_started) {
    if (callback) {
      callback(StatusCode::kOk);
    }
    return;
  }
  impl_->service->SetVisibility(
      nearby::sharing::proto::DeviceVisibility::DEVICE_VISIBILITY_EVERYONE,
      absl::Minutes(10),
      [this, cb = std::move(callback)](
          nearby::sharing::NearbySharingService::StatusCodes status) mutable {
        if (status != nearby::sharing::NearbySharingService::StatusCodes::kOk) {
          if (cb) {
            cb(ToFacadeStatus(status));
          }
          return;
        }
        impl_->service->RegisterReceiveSurface(
            impl_.get(),
            nearby::sharing::NearbySharingService::ReceiveSurfaceState::
                kForeground,
            nearby::sharing::Advertisement::BlockedVendorId::kNone,
            [this, cb = std::move(cb)](
                nearby::sharing::NearbySharingService::StatusCodes status)
                mutable {
              if (status ==
                  nearby::sharing::NearbySharingService::StatusCodes::kOk) {
                impl_->receive_mode_started = true;
              }
              if (cb) {
                cb(ToFacadeStatus(status));
              }
            });
      });
}

void NearbySharingApi::StopReceiveMode(std::function<void(StatusCode)> callback) {
  if (impl_->service == nullptr) {
    if (callback) {
      callback(StatusCode::kError);
    }
    return;
  }
  if (!impl_->receive_mode_started) {
    if (callback) {
      callback(StatusCode::kStatusAlreadyStopped);
    }
    return;
  }
  impl_->service->UnregisterReceiveSurface(
      impl_.get(),
      [this, cb = std::move(callback)](
          nearby::sharing::NearbySharingService::StatusCodes status) mutable {
        if (status == nearby::sharing::NearbySharingService::StatusCodes::kOk) {
          impl_->receive_mode_started = false;
        }
        if (cb) {
          cb(ToFacadeStatus(status));
        }
      });
}

void NearbySharingApi::SendFile(int64_t share_target_id,
                                const std::string& file_path,
                                std::function<void(StatusCode)> callback) {
  if (impl_->service == nullptr) {
    if (callback) {
      callback(StatusCode::kError);
    }
    return;
  }
  if (file_path.empty()) {
    if (callback) {
      callback(StatusCode::kInvalidArgument);
    }
    return;
  }

  FilePath path(file_path);
  std::optional<uintmax_t> file_size = nearby::Files::GetFileSize(path);
  if (!file_size.has_value() || *file_size == 0 ||
      *file_size >
          static_cast<uintmax_t>(std::numeric_limits<int64_t>::max())) {
    if (callback) {
      callback(StatusCode::kInvalidArgument);
    }
    return;
  }

  nearby::sharing::AttachmentContainer::Builder builder;
  nearby::sharing::FileAttachment attachment(path);
  attachment.set_size(static_cast<int64_t>(*file_size));
  builder.AddFileAttachment(std::move(attachment));
  std::unique_ptr<nearby::sharing::AttachmentContainer> attachments =
      builder.Build();
  if (!attachments || !attachments->HasAttachments()) {
    if (callback) {
      callback(StatusCode::kInvalidArgument);
    }
    return;
  }

  impl_->service->SendAttachments(
      share_target_id, std::move(attachments),
      [cb = std::move(callback)](
          nearby::sharing::NearbySharingService::StatusCodes status) mutable {
        if (cb) {
          cb(ToFacadeStatus(status));
        }
      });
}

void NearbySharingApi::Accept(int64_t share_target_id,
                              std::function<void(StatusCode)> callback) {
  if (impl_->service == nullptr) {
    if (callback) {
      callback(StatusCode::kError);
    }
    return;
  }
  impl_->service->Accept(
      share_target_id,
      [cb = std::move(callback)](
          nearby::sharing::NearbySharingService::StatusCodes status) mutable {
        if (cb) {
          cb(ToFacadeStatus(status));
        }
      });
}

void NearbySharingApi::Reject(int64_t share_target_id,
                              std::function<void(StatusCode)> callback) {
  if (impl_->service == nullptr) {
    if (callback) {
      callback(StatusCode::kError);
    }
    return;
  }
  impl_->service->Reject(
      share_target_id,
      [cb = std::move(callback)](
          nearby::sharing::NearbySharingService::StatusCodes status) mutable {
        if (cb) {
          cb(ToFacadeStatus(status));
        }
      });
}

void NearbySharingApi::Cancel(int64_t share_target_id,
                              std::function<void(StatusCode)> callback) {
  if (impl_->service == nullptr) {
    if (callback) {
      callback(StatusCode::kError);
    }
    return;
  }
  impl_->service->Cancel(
      share_target_id,
      [cb = std::move(callback)](
          nearby::sharing::NearbySharingService::StatusCodes status) mutable {
        if (cb) {
          cb(ToFacadeStatus(status));
        }
      });
}

void NearbySharingApi::SetDeviceName(const std::string& device_name) {
  if (impl_->service == nullptr || device_name.empty()) {
    return;
  }
  if (impl_->service->GetLocalDeviceDataManager() == nullptr) {
    return;
  }
  impl_->service->GetLocalDeviceDataManager()->SetDeviceName(device_name);
}

void NearbySharingApi::Shutdown(std::function<void(StatusCode)> callback) {
  if (impl_->service == nullptr) {
    if (callback) {
      callback(StatusCode::kError);
    }
    return;
  }
  impl_->service->Shutdown(
      [this, cb = std::move(callback)](
          nearby::sharing::NearbySharingService::StatusCodes status) mutable {
        if (status == nearby::sharing::NearbySharingService::StatusCodes::kOk) {
          impl_->send_mode_started = false;
          impl_->receive_mode_started = false;
        }
        if (cb) {
          cb(ToFacadeStatus(status));
        }
      });
}

std::string NearbySharingApi::GetQrCodeUrl() const {
  if (impl_->qr_code_url.empty()) {
    impl_->qr_code_url = GenerateQrCodeUrl();
  }
  return impl_->qr_code_url;
}

std::string NearbySharingApi::StatusCodeToString(StatusCode status) {
  switch (status) {
    case StatusCode::kOk:
      return "Ok";
    case StatusCode::kError:
      return "Error";
    case StatusCode::kOutOfOrderApiCall:
      return "OutOfOrderApiCall";
    case StatusCode::kStatusAlreadyStopped:
      return "StatusAlreadyStopped";
    case StatusCode::kTransferAlreadyInProgress:
      return "TransferAlreadyInProgress";
    case StatusCode::kNoAvailableConnectionMedium:
      return "NoAvailableConnectionMedium";
    case StatusCode::kIrrecoverableHardwareError:
      return "IrrecoverableHardwareError";
    case StatusCode::kInvalidArgument:
      return "InvalidArgument";
  }
  return "Error";
}

std::string NearbySharingApi::TransferStatusToString(TransferStatus status) {
  switch (status) {
    case TransferStatus::kUnknown:
      return "Unknown";
    case TransferStatus::kConnecting:
      return "Connecting";
    case TransferStatus::kAwaitingLocalConfirmation:
      return "AwaitingLocalConfirmation";
    case TransferStatus::kAwaitingRemoteAcceptance:
      return "AwaitingRemoteAcceptance";
    case TransferStatus::kInProgress:
      return "InProgress";
    case TransferStatus::kComplete:
      return "Complete";
    case TransferStatus::kFailed:
      return "Failed";
    case TransferStatus::kRejected:
      return "Rejected";
    case TransferStatus::kCancelled:
      return "Cancelled";
    case TransferStatus::kTimedOut:
      return "TimedOut";
    case TransferStatus::kMediaUnavailable:
      return "MediaUnavailable";
    case TransferStatus::kNotEnoughSpace:
      return "NotEnoughSpace";
    case TransferStatus::kUnsupportedAttachmentType:
      return "UnsupportedAttachmentType";
    case TransferStatus::kDeviceAuthenticationFailed:
      return "DeviceAuthenticationFailed";
    case TransferStatus::kIncompletePayloads:
      return "IncompletePayloads";
  }
  return "Unknown";
}

}  // namespace nearby::sharing
