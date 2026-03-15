#include "sharing/linux/nearby_sharing_api.h"

#include <limits>
#include <mutex>
#include <optional>
#include <utility>

#include "connections/implementation/flags/nearby_connections_feature_flags.h"
#include "internal/base/file_path.h"
#include "internal/base/files.h"
#include "internal/flags/nearby_flags.h"
#include "sharing/attachment_container.h"
#include "sharing/file_attachment.h"
#include "sharing/linux/nearby_sharing_service_linux.h"
#include "sharing/share_target_discovered_callback.h"
#include "sharing/transfer_metadata.h"
#include "sharing/transfer_update_callback.h"

namespace nearby::sharing::linux {

namespace {

using NativeService = nearby::sharing::linux::NearbySharingServiceLinux;

void EnableBleL2capDefaults() {
  nearby::NearbyFlags::GetInstance().OverrideBoolFlagValue(
      nearby::connections::config_package_nearby::nearby_connections_feature::
          kEnableBleL2cap,
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

}  // namespace

class NearbySharingApi::Impl : public nearby::sharing::ShareTargetDiscoveredCallback,
                               public nearby::sharing::TransferUpdateCallback {
 public:
  Impl() : service() {}
  explicit Impl(std::string device_name_override)
      : service(std::move(device_name_override)) {}

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
    info.progress = transfer_metadata.progress();
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

  NativeService service;
  bool send_mode_started = false;
  bool receive_mode_started = false;
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
  if (impl_->send_mode_started) {
    if (callback) {
      callback(StatusCode::kOk);
    }
    return;
  }
  impl_->service.RegisterSendSurface(
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
  if (!impl_->send_mode_started) {
    if (callback) {
      callback(StatusCode::kStatusAlreadyStopped);
    }
    return;
  }
  impl_->service.UnregisterSendSurface(
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
  if (impl_->receive_mode_started) {
    if (callback) {
      callback(StatusCode::kOk);
    }
    return;
  }
  impl_->service.RegisterReceiveSurface(
      impl_.get(),
      nearby::sharing::NearbySharingService::ReceiveSurfaceState::kForeground,
      nearby::sharing::Advertisement::BlockedVendorId::kNone,
      [this, cb = std::move(callback)](
          nearby::sharing::NearbySharingService::StatusCodes status) mutable {
        if (status == nearby::sharing::NearbySharingService::StatusCodes::kOk) {
          impl_->receive_mode_started = true;
        }
        if (cb) {
          cb(ToFacadeStatus(status));
        }
      });
}

void NearbySharingApi::StopReceiveMode(std::function<void(StatusCode)> callback) {
  if (!impl_->receive_mode_started) {
    if (callback) {
      callback(StatusCode::kStatusAlreadyStopped);
    }
    return;
  }
  impl_->service.UnregisterReceiveSurface(
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

  impl_->service.SendAttachments(
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
  impl_->service.Accept(
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
  impl_->service.Reject(
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
  impl_->service.Cancel(
      share_target_id,
      [cb = std::move(callback)](
          nearby::sharing::NearbySharingService::StatusCodes status) mutable {
        if (cb) {
          cb(ToFacadeStatus(status));
        }
      });
}

void NearbySharingApi::Shutdown(std::function<void(StatusCode)> callback) {
  impl_->service.Shutdown(
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
  return impl_->service.GetQrCodeUrl();
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

}  // namespace nearby::sharing::linux
