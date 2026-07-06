#include "sharing/linux/daemon/nearby_sharing_dbus_service.h"

#include <condition_variable>
#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <utility>
#include <vector>

#include "internal/base/file_path.h"
#include "sharing/advertisement.h"
#include "sharing/file_attachment.h"

namespace nearby::sharing::linux {
namespace {

std::string StatusCodeToString(NearbySharingService::StatusCodes status) {
  return NearbySharingService::StatusCodeToString(status);
}

std::tuple<bool, std::string> StatusResult(
    NearbySharingService::StatusCodes status) {
  return {status == NearbySharingService::StatusCodes::kOk,
          StatusCodeToString(status)};
}

std::unique_ptr<AttachmentContainer> CreateFileAttachments(
    const std::string& file_path) {
  AttachmentContainer::Builder builder;
  builder.AddFileAttachment(FileAttachment(FilePath(file_path)));
  return builder.Build();
}

template <typename Invoker>
NearbySharingService::StatusCodes WaitForStatus(Invoker invoker) {
  std::mutex mutex;
  std::condition_variable cv;
  std::optional<NearbySharingService::StatusCodes> status;
  invoker([&](NearbySharingService::StatusCodes callback_status) {
    {
      std::lock_guard<std::mutex> lock(mutex);
      status = callback_status;
    }
    cv.notify_one();
  });

  std::unique_lock<std::mutex> lock(mutex);
  cv.wait(lock, [&] { return status.has_value(); });
  return *status;
}

}  // namespace

DbusDictionary ShareTargetToDbus(const ShareTarget& share_target) {
  DbusDictionary result;
  result.emplace("id", sdbus::Variant(share_target.id));
  result.emplace("device_name", sdbus::Variant(share_target.device_name));
  result.emplace("type",
                 sdbus::Variant(static_cast<int32_t>(share_target.type)));
  result.emplace("is_incoming", sdbus::Variant(share_target.is_incoming));
  result.emplace("is_known", sdbus::Variant(share_target.is_known));
  result.emplace("device_id", sdbus::Variant(share_target.device_id));
  result.emplace("for_self_share", sdbus::Variant(share_target.for_self_share));
  result.emplace("vendor_id",
                 sdbus::Variant(static_cast<int32_t>(share_target.vendor_id)));
  result.emplace("receive_disabled",
                 sdbus::Variant(share_target.receive_disabled));
  return result;
}

DbusDictionary TransferMetadataToDbus(
    const TransferMetadata& transfer_metadata,
    const AttachmentContainer& attachment_container) {
  DbusDictionary result;
  result.emplace("status", sdbus::Variant(TransferMetadata::StatusToString(
                               transfer_metadata.status())));
  result.emplace(
      "progress",
      sdbus::Variant(static_cast<double>(transfer_metadata.progress())));
  result.emplace("transferred_bytes",
                 sdbus::Variant(static_cast<int64_t>(
                     transfer_metadata.transferred_bytes())));
  result.emplace(
      "total_bytes",
      sdbus::Variant(attachment_container.GetTotalAttachmentsSize()));
  result.emplace(
      "transfer_speed",
      sdbus::Variant(static_cast<int64_t>(transfer_metadata.transfer_speed())));
  result.emplace("estimated_time_remaining",
                 sdbus::Variant(static_cast<int64_t>(
                     transfer_metadata.estimated_time_remaining())));
  result.emplace("total_attachments_count",
                 sdbus::Variant(transfer_metadata.total_attachments_count()));
  result.emplace(
      "transferred_attachments_count",
      sdbus::Variant(transfer_metadata.transferred_attachments_count()));
  result.emplace("is_final_status",
                 sdbus::Variant(transfer_metadata.is_final_status()));
  result.emplace("is_self_share",
                 sdbus::Variant(transfer_metadata.is_self_share()));
  result.emplace("binding_id", sdbus::Variant(transfer_metadata.binding_id()));

  if (transfer_metadata.token().has_value()) {
    result.emplace("token", sdbus::Variant(*transfer_metadata.token()));
  }
  if (transfer_metadata.in_progress_attachment_id().has_value()) {
    result.emplace(
        "in_progress_attachment_id",
        sdbus::Variant(*transfer_metadata.in_progress_attachment_id()));
  }
  if (transfer_metadata.in_progress_attachment_transferred_bytes()
          .has_value()) {
    result.emplace(
        "in_progress_attachment_transferred_bytes",
        sdbus::Variant(static_cast<int64_t>(
            *transfer_metadata.in_progress_attachment_transferred_bytes())));
  }
  if (transfer_metadata.in_progress_attachment_total_bytes().has_value()) {
    result.emplace(
        "in_progress_attachment_total_bytes",
        sdbus::Variant(static_cast<int64_t>(
            *transfer_metadata.in_progress_attachment_total_bytes())));
  }
  return result;
}

NearbySharingDbusService::TransferCallback::TransferCallback(
    NearbySharingDbusService& service, bool receive_mode)
    : service_(service), receive_mode_(receive_mode) {}

void NearbySharingDbusService::TransferCallback::OnTransferUpdate(
    const ShareTarget& share_target,
    const AttachmentContainer& attachment_container,
    const TransferMetadata& transfer_metadata) {
  service_.OnTransferUpdate(receive_mode_, share_target, attachment_container,
                            transfer_metadata);
}

NearbySharingDbusService::NearbySharingDbusService(
    sdbus::IObject& object, NearbySharingService& service,
    QuitCallback quit_callback)
    : sharing_adaptor(object),
      service_(service),
      quit_callback_(std::move(quit_callback)),
      send_transfer_callback_(*this, /*receive_mode=*/false),
      receive_transfer_callback_(*this, /*receive_mode=*/true) {
  registerAdaptor();
  service_.AddObserver(this);
}

NearbySharingDbusService::~NearbySharingDbusService() {
  service_.RemoveObserver(this);
  ShutdownService();
}

void NearbySharingDbusService::EmitStatusChanged() {
  emitStatusChanged(StatusToDbus());
}

void NearbySharingDbusService::ShutdownService() {
  bool stop_receive = false;
  bool stop_discovery = false;
  {
    absl::MutexLock lock(lock_);
    if (shutdown_) {
      return;
    }
    shutdown_ = true;
    stop_receive = receive_registered_;
    stop_discovery = discovery_registered_;
    receive_registered_ = false;
    discovery_registered_ = false;
    targets_.clear();
  }

  if (stop_receive) {
    WaitForStatus([&](auto callback) {
      service_.UnregisterReceiveSurface(&receive_transfer_callback_,
                                        std::move(callback));
    });
  }
  if (stop_discovery) {
    WaitForStatus([&](auto callback) {
      service_.UnregisterSendSurface(&send_transfer_callback_,
                                     std::move(callback));
    });
  }
  WaitForStatus([&](auto callback) { service_.Shutdown(std::move(callback)); });
  EmitStatusChanged();
}

std::tuple<bool, std::string> NearbySharingDbusService::StartReceive() {
  {
    absl::MutexLock lock(lock_);
    if (receive_registered_) {
      return {true, "receive already started"};
    }
  }

  auto result = InvokeStatusCommand([&](auto callback) {
    service_.RegisterReceiveSurface(
        &receive_transfer_callback_,
        NearbySharingService::ReceiveSurfaceState::kForeground,
        Advertisement::BlockedVendorId::kNone, std::move(callback));
  });
  if (std::get<0>(result)) {
    {
      absl::MutexLock lock(lock_);
      receive_registered_ = true;
    }
    EmitStatusChanged();
  }
  return result;
}

std::tuple<bool, std::string> NearbySharingDbusService::StopReceive() {
  {
    absl::MutexLock lock(lock_);
    if (!receive_registered_) {
      return {true, "receive already stopped"};
    }
  }

  auto result = InvokeStatusCommand([&](auto callback) {
    service_.UnregisterReceiveSurface(&receive_transfer_callback_,
                                      std::move(callback));
  });
  if (std::get<0>(result)) {
    {
      absl::MutexLock lock(lock_);
      receive_registered_ = false;
    }
    EmitStatusChanged();
  }
  return result;
}

std::tuple<bool, std::string> NearbySharingDbusService::StartDiscovery() {
  {
    absl::MutexLock lock(lock_);
    if (discovery_registered_) {
      return {true, "discovery already started"};
    }
  }

  auto result = InvokeStatusCommand([&](auto callback) {
    service_.RegisterSendSurface(
        &send_transfer_callback_, this,
        NearbySharingService::SendSurfaceState::kForeground,
        Advertisement::BlockedVendorId::kNone,
        /*disable_wifi_hotspot=*/false, std::move(callback));
  });
  if (std::get<0>(result)) {
    {
      absl::MutexLock lock(lock_);
      discovery_registered_ = true;
    }
    EmitStatusChanged();
  }
  return result;
}

std::tuple<bool, std::string> NearbySharingDbusService::StopDiscovery() {
  {
    absl::MutexLock lock(lock_);
    if (!discovery_registered_) {
      return {true, "discovery already stopped"};
    }
  }

  auto result = InvokeStatusCommand([&](auto callback) {
    service_.UnregisterSendSurface(&send_transfer_callback_,
                                   std::move(callback));
  });
  if (std::get<0>(result)) {
    {
      absl::MutexLock lock(lock_);
      discovery_registered_ = false;
      targets_.clear();
    }
    EmitStatusChanged();
  }
  return result;
}

std::tuple<bool, std::string> NearbySharingDbusService::SendFile(
    const int64_t& share_target_id, const std::string& path) {
  std::error_code file_error;
  if (!std::filesystem::is_regular_file(path, file_error)) {
    return {false, "path is not a regular file"};
  }

  {
    absl::MutexLock lock(lock_);
    if (targets_.find(share_target_id) == targets_.end()) {
      return {false, "unknown share_target_id"};
    }
  }

  auto attachments = CreateFileAttachments(path);
  return InvokeStatusCommand([&](auto callback) {
    service_.SendAttachments(share_target_id, std::move(attachments),
                             std::move(callback));
  });
}

std::tuple<bool, std::string> NearbySharingDbusService::Accept(
    const int64_t& share_target_id) {
  LOG(INFO) << "Accept requested for target: " << share_target_id;

  service_.Accept(
      share_target_id,
      [this, share_target_id](NearbySharingService::StatusCodes status) {
        LOG(INFO) << "Accept completed for target " << share_target_id
                  << " with status: "
                  << NearbySharingService::StatusCodeToString(status);

        EmitStatusChanged();
      });

  return {true, "accept requested"};
}

std::tuple<bool, std::string> NearbySharingDbusService::Reject(
    const int64_t& share_target_id) {
  return InvokeStatusCommand([&](auto callback) {
    service_.Reject(share_target_id, std::move(callback));
  });
}

std::tuple<bool, std::string> NearbySharingDbusService::Cancel(
    const int64_t& share_target_id) {
  return InvokeStatusCommand([&](auto callback) {
    service_.Cancel(share_target_id, std::move(callback));
  });
}

std::tuple<bool, std::string> NearbySharingDbusService::Shutdown() {
  ShutdownService();
  if (quit_callback_) {
    quit_callback_();
  }
  return {true, "daemon service shut down"};
}

std::tuple<bool, std::string> NearbySharingDbusService::InvokeStatusCommand(
    std::function<void(std::function<void(NearbySharingService::StatusCodes)>)>
        invoker) {
  return StatusResult(WaitForStatus(std::move(invoker)));
}

DbusDictionary NearbySharingDbusService::StatusToDbus() const {
  std::vector<DbusDictionary> targets;
  bool receive_registered = false;
  bool discovery_registered = false;
  {
    absl::MutexLock lock(lock_);
    receive_registered = receive_registered_;
    discovery_registered = discovery_registered_;
    targets.reserve(targets_.size());
    for (const auto& [id, target] : targets_) {
      static_cast<void>(id);
      targets.push_back(ShareTargetToDbus(target));
    }
  }

  DbusDictionary status;
  status.emplace("receive_registered", sdbus::Variant(receive_registered));
  status.emplace("discovery_registered", sdbus::Variant(discovery_registered));
  status.emplace("is_transferring", sdbus::Variant(service_.IsTransferring()));
  status.emplace("is_scanning", sdbus::Variant(service_.IsScanning()));
  status.emplace("bluetooth_present",
                 sdbus::Variant(service_.IsBluetoothPresent()));
  status.emplace("bluetooth_powered",
                 sdbus::Variant(service_.IsBluetoothPowered()));
  status.emplace("lan_connected", sdbus::Variant(service_.IsLanConnected()));
  status.emplace("targets", sdbus::Variant(targets));
  return status;
}

void NearbySharingDbusService::OnTransferUpdate(
    bool receive_mode, const ShareTarget& share_target,
    const AttachmentContainer& attachment_container,
    const TransferMetadata& transfer_metadata) {
  const std::string direction = receive_mode ? "receive" : "send";
  const DbusDictionary target = ShareTargetToDbus(share_target);
  const DbusDictionary transfer =
      TransferMetadataToDbus(transfer_metadata, attachment_container);

  if (receive_mode &&
      transfer_metadata.status() ==
          TransferMetadata::Status::kAwaitingLocalConfirmation) {
    emitIncomingTransfer(direction, target, transfer);
  } else {
    emitTransferUpdate(direction, target, transfer);
  }
  EmitStatusChanged();
}

void NearbySharingDbusService::OnShareTargetDiscovered(
    const ShareTarget& share_target) {
  {
    absl::MutexLock lock(lock_);
    targets_[share_target.id] = share_target;
  }
  emitTargetDiscovered(ShareTargetToDbus(share_target));
  EmitStatusChanged();
}

void NearbySharingDbusService::OnShareTargetLost(
    const ShareTarget& share_target) {
  {
    absl::MutexLock lock(lock_);
    targets_.erase(share_target.id);
  }
  emitTargetLost(ShareTargetToDbus(share_target));
  EmitStatusChanged();
}

void NearbySharingDbusService::OnShareTargetUpdated(
    const ShareTarget& share_target) {
  {
    absl::MutexLock lock(lock_);
    targets_[share_target.id] = share_target;
  }
  emitTargetUpdated(ShareTargetToDbus(share_target));
  EmitStatusChanged();
}

void NearbySharingDbusService::OnHighVisibilityChanged(
    bool in_high_visibility) {
  static_cast<void>(in_high_visibility);
  EmitStatusChanged();
}

void NearbySharingDbusService::OnBluetoothStatusChanged(AdapterState state) {
  static_cast<void>(state);
  EmitStatusChanged();
}

void NearbySharingDbusService::OnLanStatusChanged(AdapterState state) {
  static_cast<void>(state);
  EmitStatusChanged();
}

void NearbySharingDbusService::OnIrrecoverableHardwareErrorReported() {
  EmitStatusChanged();
}

}  // namespace nearby::sharing::linux
