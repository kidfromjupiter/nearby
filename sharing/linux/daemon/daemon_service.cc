#include "sharing/linux/daemon/daemon_service.h"

#include <condition_variable>
#include <filesystem>
#include <mutex>
#include <utility>

#include "absl/time/time.h"
#include "internal/base/file_path.h"
#include "sharing/advertisement.h"
#include "sharing/file_attachment.h"

namespace nearby::sharing::linux {
namespace {

nlohmann::json CommandResult(std::string command, bool ok,
                             std::string message) {
  return nlohmann::json{{"event", "command_result"},
                        {"command", std::move(command)},
                        {"ok", ok},
                        {"message", std::move(message)}};
}

std::string StatusCodeToString(NearbySharingService::StatusCodes status) {
  return NearbySharingService::StatusCodeToString(status);
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

std::unique_ptr<AttachmentContainer> CreateFileAttachments(
    const std::string& file_path) {
  AttachmentContainer::Builder builder;
  builder.AddFileAttachment(FileAttachment(FilePath(file_path)));
  return builder.Build();
}

}  // namespace

nlohmann::json ShareTargetToJson(const ShareTarget& share_target) {
  return nlohmann::json{
      {"id", share_target.id},
      {"device_name", share_target.device_name},
      {"type", static_cast<int>(share_target.type)},
      {"is_incoming", share_target.is_incoming},
      {"is_known", share_target.is_known},
      {"device_id", share_target.device_id},
      {"for_self_share", share_target.for_self_share},
      {"vendor_id", share_target.vendor_id},
      {"receive_disabled", share_target.receive_disabled},
  };
}

nlohmann::json TransferMetadataToJson(
    const TransferMetadata& transfer_metadata,
    const AttachmentContainer& attachment_container) {
  nlohmann::json result{
      {"status", TransferMetadata::StatusToString(transfer_metadata.status())},
      {"progress", transfer_metadata.progress()},
      {"transferred_bytes", transfer_metadata.transferred_bytes()},
      {"total_bytes", attachment_container.GetTotalAttachmentsSize()},
      {"transfer_speed", transfer_metadata.transfer_speed()},
      {"estimated_time_remaining",
       transfer_metadata.estimated_time_remaining()},
      {"total_attachments_count", transfer_metadata.total_attachments_count()},
      {"transferred_attachments_count",
       transfer_metadata.transferred_attachments_count()},
      {"is_final_status", transfer_metadata.is_final_status()},
      {"is_self_share", transfer_metadata.is_self_share()},
      {"binding_id", transfer_metadata.binding_id()},
  };
  if (transfer_metadata.token().has_value()) {
    result["token"] = *transfer_metadata.token();
  }
  if (transfer_metadata.in_progress_attachment_id().has_value()) {
    result["in_progress_attachment_id"] =
        *transfer_metadata.in_progress_attachment_id();
  }
  if (transfer_metadata.in_progress_attachment_transferred_bytes()
          .has_value()) {
    result["in_progress_attachment_transferred_bytes"] =
        *transfer_metadata.in_progress_attachment_transferred_bytes();
  }
  if (transfer_metadata.in_progress_attachment_total_bytes().has_value()) {
    result["in_progress_attachment_total_bytes"] =
        *transfer_metadata.in_progress_attachment_total_bytes();
  }
  return result;
}

DaemonService::DaemonService(NearbySharingService& service,
                             EventSink event_sink)
    : service_(service),
      event_sink_(std::move(event_sink)),
      send_transfer_callback_(*this, /*receive_mode=*/false),
      receive_transfer_callback_(*this, /*receive_mode=*/true),
      discovery_callback_(*this) {}

DaemonService::~DaemonService() { Shutdown(); }

nlohmann::json DaemonService::HandleCommand(const nlohmann::json& request) {
  if (!request.is_object() || !request.contains("command") ||
      !request["command"].is_string()) {
    return CommandResult("", false, "missing string field: command");
  }
  std::string command = request["command"].get<std::string>();
  if (command == "status") {
    return Status();
  }
  if (command == "start_receive") {
    return StartReceive();
  }
  if (command == "stop_receive") {
    return StopReceive();
  }
  if (command == "start_discovery") {
    return StartDiscovery();
  }
  if (command == "stop_discovery") {
    return StopDiscovery();
  }
  if (command == "send_file") {
    return SendFile(request);
  }
  if (command == "accept") {
    return Accept(request);
  }
  if (command == "reject") {
    return Reject(request);
  }
  if (command == "cancel") {
    return Cancel(request);
  }
  if (command == "shutdown") {
    Shutdown();
    return CommandResult(command, true, "daemon service shut down");
  }
  return CommandResult(command, false, "unknown command");
}

void DaemonService::Shutdown() {
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
    active_transfers_.clear();
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
}

nlohmann::json DaemonService::StartReceive() {
  {
    absl::MutexLock lock(lock_);
    if (receive_registered_) {
      return CommandResult("start_receive", true, "receive already started");
    }
  }

  auto result = InvokeStatusCommand("start_receive", [&](auto callback) {
    service_.RegisterReceiveSurface(
        &receive_transfer_callback_,
        NearbySharingService::ReceiveSurfaceState::kForeground,
        Advertisement::BlockedVendorId::kNone, std::move(callback));
  });
  if (result["ok"].get<bool>()) {
    absl::MutexLock lock(lock_);
    receive_registered_ = true;
  }
  return result;
}

nlohmann::json DaemonService::StopReceive() {
  {
    absl::MutexLock lock(lock_);
    if (!receive_registered_) {
      return CommandResult("stop_receive", true, "receive already stopped");
    }
    receive_registered_ = false;
  }

  return InvokeStatusCommand("stop_receive", [&](auto callback) {
    service_.UnregisterReceiveSurface(&receive_transfer_callback_,
                                      std::move(callback));
  });
}

nlohmann::json DaemonService::StartDiscovery() {
  {
    absl::MutexLock lock(lock_);
    if (discovery_registered_) {
      return CommandResult("start_discovery", true, "discovery already started");
    }
  }

  auto result = InvokeStatusCommand("start_discovery", [&](auto callback) {
    service_.RegisterSendSurface(
        &send_transfer_callback_, &discovery_callback_,
        NearbySharingService::SendSurfaceState::kForeground,
        Advertisement::BlockedVendorId::kNone,
        /*disable_wifi_hotspot=*/false, std::move(callback));
  });
  if (result["ok"].get<bool>()) {
    absl::MutexLock lock(lock_);
    discovery_registered_ = true;
  }
  return result;
}

nlohmann::json DaemonService::StopDiscovery() {
  {
    absl::MutexLock lock(lock_);
    if (!discovery_registered_) {
      return CommandResult("stop_discovery", true, "discovery already stopped");
    }
    discovery_registered_ = false;
    targets_.clear();
  }

  return InvokeStatusCommand("stop_discovery", [&](auto callback) {
    service_.UnregisterSendSurface(&send_transfer_callback_,
                                   std::move(callback));
  });
}

nlohmann::json DaemonService::SendFile(const nlohmann::json& request) {
  nlohmann::json error;
  std::optional<int64_t> share_target_id = GetRequiredTargetId(request, error);
  if (!share_target_id.has_value()) {
    error["command"] = "send_file";
    return error;
  }
  if (!request.contains("path") || !request["path"].is_string()) {
    return CommandResult("send_file", false, "missing string field: path");
  }

  std::string path = request["path"].get<std::string>();
  std::error_code file_error;
  if (!std::filesystem::is_regular_file(path, file_error)) {
    return CommandResult("send_file", false, "path is not a regular file");
  }

  {
    absl::MutexLock lock(lock_);
    if (targets_.find(*share_target_id) == targets_.end()) {
      return CommandResult("send_file", false, "unknown share_target_id");
    }
  }

  auto attachments = CreateFileAttachments(path);
  return InvokeStatusCommand("send_file", [&](auto callback) {
    service_.SendAttachments(*share_target_id, std::move(attachments),
                             std::move(callback));
  });
}

nlohmann::json DaemonService::Accept(const nlohmann::json& request) {
  nlohmann::json error;
  std::optional<int64_t> share_target_id = GetRequiredTargetId(request, error);
  if (!share_target_id.has_value()) {
    error["command"] = "accept";
    return error;
  }
  return InvokeStatusCommand("accept", [&](auto callback) {
    service_.Accept(*share_target_id, std::move(callback));
  });
}

nlohmann::json DaemonService::Reject(const nlohmann::json& request) {
  nlohmann::json error;
  std::optional<int64_t> share_target_id = GetRequiredTargetId(request, error);
  if (!share_target_id.has_value()) {
    error["command"] = "reject";
    return error;
  }
  return InvokeStatusCommand("reject", [&](auto callback) {
    service_.Reject(*share_target_id, std::move(callback));
  });
}

nlohmann::json DaemonService::Cancel(const nlohmann::json& request) {
  nlohmann::json error;
  std::optional<int64_t> share_target_id = GetRequiredTargetId(request, error);
  if (!share_target_id.has_value()) {
    error["command"] = "cancel";
    return error;
  }
  return InvokeStatusCommand("cancel", [&](auto callback) {
    service_.Cancel(*share_target_id, std::move(callback));
  });
}

nlohmann::json DaemonService::Status() {
  nlohmann::json targets = nlohmann::json::array();
  {
    absl::MutexLock lock(lock_);
    for (const auto& [id, target] : targets_) {
      static_cast<void>(id);
      targets.push_back(ShareTargetToJson(target));
    }
    return nlohmann::json{{"event", "command_result"},
                          {"command", "status"},
                          {"ok", true},
                          {"receive_registered", receive_registered_},
                          {"discovery_registered", discovery_registered_},
                          {"is_transferring", service_.IsTransferring()},
                          {"is_scanning", service_.IsScanning()},
                          {"bluetooth_present", service_.IsBluetoothPresent()},
                          {"bluetooth_powered", service_.IsBluetoothPowered()},
                          {"lan_connected", service_.IsLanConnected()},
                          {"targets", std::move(targets)}};
  }
}

void DaemonService::OnTransferUpdate(
    bool receive_mode, const ShareTarget& share_target,
    const AttachmentContainer& attachment_container,
    const TransferMetadata& transfer_metadata) {
  {
    absl::MutexLock lock(lock_);
    active_transfers_[share_target.id] = share_target;
    if (TransferMetadata::IsFinalStatus(transfer_metadata.status())) {
      active_transfers_.erase(share_target.id);
    }
  }

  std::string event =
      receive_mode &&
              transfer_metadata.status() ==
                  TransferMetadata::Status::kAwaitingLocalConfirmation
          ? "incoming_transfer"
          : "transfer_update";
  Emit(nlohmann::json{
      {"event", event},
      {"direction", receive_mode ? "receive" : "send"},
      {"share_target", ShareTargetToJson(share_target)},
      {"transfer",
       TransferMetadataToJson(transfer_metadata, attachment_container)},
  });
}

void DaemonService::OnShareTargetDiscovered(const ShareTarget& share_target) {
  {
    absl::MutexLock lock(lock_);
    targets_[share_target.id] = share_target;
  }
  Emit(nlohmann::json{{"event", "target_discovered"},
                      {"share_target", ShareTargetToJson(share_target)}});
}

void DaemonService::OnShareTargetUpdated(const ShareTarget& share_target) {
  {
    absl::MutexLock lock(lock_);
    targets_[share_target.id] = share_target;
  }
  Emit(nlohmann::json{{"event", "target_updated"},
                      {"share_target", ShareTargetToJson(share_target)}});
}

void DaemonService::OnShareTargetLost(const ShareTarget& share_target) {
  {
    absl::MutexLock lock(lock_);
    targets_.erase(share_target.id);
  }
  Emit(nlohmann::json{{"event", "target_lost"},
                      {"share_target", ShareTargetToJson(share_target)}});
}

nlohmann::json DaemonService::InvokeStatusCommand(
    const std::string& command,
    std::function<void(std::function<void(NearbySharingService::StatusCodes)>)>
        invoker) {
  NearbySharingService::StatusCodes status = WaitForStatus(std::move(invoker));
  bool ok = status == NearbySharingService::StatusCodes::kOk;
  return CommandResult(command, ok, StatusCodeToString(status));
}

std::optional<int64_t> DaemonService::GetRequiredTargetId(
    const nlohmann::json& request, nlohmann::json& error) const {
  if (!request.contains("share_target_id") ||
      !request["share_target_id"].is_number_integer()) {
    error = CommandResult("", false, "missing integer field: share_target_id");
    return std::nullopt;
  }
  return request["share_target_id"].get<int64_t>();
}

void DaemonService::Emit(nlohmann::json event) {
  if (event_sink_) {
    event_sink_(std::move(event));
  }
}

void DaemonService::TransferCallback::OnTransferUpdate(
    const ShareTarget& share_target,
    const AttachmentContainer& attachment_container,
    const TransferMetadata& transfer_metadata) {
  daemon_.OnTransferUpdate(receive_mode_, share_target, attachment_container,
                           transfer_metadata);
}

void DaemonService::DiscoveryCallback::OnShareTargetDiscovered(
    const ShareTarget& share_target) {
  daemon_.OnShareTargetDiscovered(share_target);
}

void DaemonService::DiscoveryCallback::OnShareTargetLost(
    const ShareTarget& share_target) {
  daemon_.OnShareTargetLost(share_target);
}

void DaemonService::DiscoveryCallback::OnShareTargetUpdated(
    const ShareTarget& share_target) {
  daemon_.OnShareTargetUpdated(share_target);
}

}  // namespace nearby::sharing::linux
