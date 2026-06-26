#include "sharing/linux/app/nearby_sharing_dbus_client.h"

#include <utility>

namespace nearby::sharing::linux::app {
namespace {

template <typename T>
T GetField(const DbusDictionary& map, const std::string& key,
           T default_value = T{}) {
  auto it = map.find(key);
  if (it == map.end()) {
    return default_value;
  }

  try {
    return it->second.get<T>();
  } catch (const sdbus::Error&) {
    return default_value;
  }
}

template <typename T>
std::optional<T> GetOptionalField(const DbusDictionary& map,
                                  const std::string& key) {
  auto it = map.find(key);
  if (it == map.end()) {
    return std::nullopt;
  }

  try {
    return it->second.get<T>();
  } catch (const sdbus::Error&) {
    return std::nullopt;
  }
}

double GetProgress(const DbusDictionary& map) {
  if (auto progress = GetOptionalField<double>(map, "progress")) {
    return *progress;
  }
  if (auto progress = GetOptionalField<int32_t>(map, "progress")) {
    return *progress;
  }
  if (auto progress = GetOptionalField<int64_t>(map, "progress")) {
    return *progress;
  }
  return 0;
}

}  // namespace

NearbySharingDbusClient::NearbySharingDbusClient(Observer* observer)
    : sdbus::ProxyInterfaces<com::google::nearby::sharing_proxy>(
          sdbus::ServiceName("com.google.nearby.sharing"),
          sdbus::ObjectPath("/com/google/nearby/sharing")),
      observer_(observer) {
  registerProxy();
}

NearbySharingDbusClient::~NearbySharingDbusClient() {
  unregisterProxy();
}

std::tuple<bool, std::string> NearbySharingDbusClient::StartReceive() {
  return com::google::nearby::sharing_proxy::StartReceive();
}

std::tuple<bool, std::string> NearbySharingDbusClient::StopReceive() {
  return com::google::nearby::sharing_proxy::StopReceive();
}

std::tuple<bool, std::string> NearbySharingDbusClient::StartDiscovery() {
  return com::google::nearby::sharing_proxy::StartDiscovery();
}

std::tuple<bool, std::string> NearbySharingDbusClient::StopDiscovery() {
  return com::google::nearby::sharing_proxy::StopDiscovery();
}

std::tuple<bool, std::string> NearbySharingDbusClient::SendFile(
    int64_t share_target_id, const std::string& path) {
  return com::google::nearby::sharing_proxy::SendFile(share_target_id, path);
}

std::tuple<bool, std::string> NearbySharingDbusClient::Accept(
    int64_t share_target_id) {
  return com::google::nearby::sharing_proxy::Accept(share_target_id);
}

std::tuple<bool, std::string> NearbySharingDbusClient::Reject(
    int64_t share_target_id) {
  return com::google::nearby::sharing_proxy::Reject(share_target_id);
}

std::tuple<bool, std::string> NearbySharingDbusClient::Cancel(
    int64_t share_target_id) {
  return com::google::nearby::sharing_proxy::Cancel(share_target_id);
}

ShareTarget NearbySharingDbusClient::ConvertToShareTarget(
    const DbusDictionary& map) {
  ShareTarget target;
  target.id = GetField<int64_t>(map, "id");
  target.device_name = GetField<std::string>(map, "device_name");
  target.type = GetField<int32_t>(map, "type");
  target.is_incoming = GetField<bool>(map, "is_incoming");
  target.is_known = GetField<bool>(map, "is_known");
  target.device_id = GetField<std::string>(map, "device_id");
  target.for_self_share = GetField<bool>(map, "for_self_share");
  target.vendor_id = GetField<int32_t>(map, "vendor_id");
  target.receive_disabled = GetField<bool>(map, "receive_disabled");
  return target;
}

Transfer NearbySharingDbusClient::ConvertToTransfer(const DbusDictionary& map) {
  Transfer transfer;
  transfer.status = GetField<std::string>(map, "status");
  transfer.progress = GetProgress(map);
  transfer.transferred_bytes = GetField<int64_t>(map, "transferred_bytes");
  transfer.total_bytes = GetField<int64_t>(map, "total_bytes");
  transfer.transfer_speed = GetField<int64_t>(map, "transfer_speed");
  transfer.estimated_time_remaining =
      GetField<int64_t>(map, "estimated_time_remaining");
  transfer.total_attachments_count =
      GetField<int32_t>(map, "total_attachments_count");
  transfer.transferred_attachments_count =
      GetField<int32_t>(map, "transferred_attachments_count");
  transfer.is_final_status = GetField<bool>(map, "is_final_status");
  transfer.is_self_share = GetField<bool>(map, "is_self_share");
  transfer.binding_id = GetField<std::string>(map, "binding_id");
  transfer.token = GetOptionalField<std::string>(map, "token");
  transfer.in_progress_attachment_id =
      GetOptionalField<int64_t>(map, "in_progress_attachment_id");
  transfer.in_progress_attachment_transferred_bytes = GetOptionalField<int64_t>(
      map, "in_progress_attachment_transferred_bytes");
  transfer.in_progress_attachment_total_bytes =
      GetOptionalField<int64_t>(map, "in_progress_attachment_total_bytes");
  return transfer;
}

Status NearbySharingDbusClient::ConvertToStatus(const DbusDictionary& map) {
  Status status;
  status.receive_registered = GetField<bool>(map, "receive_registered");
  status.discovery_registered = GetField<bool>(map, "discovery_registered");
  status.is_transferring = GetField<bool>(map, "is_transferring");
  status.is_scanning = GetField<bool>(map, "is_scanning");
  status.bluetooth_present = GetField<bool>(map, "bluetooth_present");
  status.bluetooth_powered = GetField<bool>(map, "bluetooth_powered");
  status.lan_connected = GetField<bool>(map, "lan_connected");

  auto raw_targets =
      GetOptionalField<std::vector<DbusDictionary>>(map, "targets");
  if (raw_targets.has_value()) {
    status.targets.reserve(raw_targets->size());
    for (const auto& target : *raw_targets) {
      status.targets.push_back(ConvertToShareTarget(target));
    }
  }

  return status;
}

void NearbySharingDbusClient::onTargetDiscovered(
    const DbusDictionary& share_target) {
  if (observer_ == nullptr) {
    return;
  }
  observer_->OnTargetDiscovered(ConvertToShareTarget(share_target));
}

void NearbySharingDbusClient::onTargetUpdated(
    const DbusDictionary& share_target) {
  if (observer_ == nullptr) {
    return;
  }
  observer_->OnTargetUpdated(ConvertToShareTarget(share_target));
}

void NearbySharingDbusClient::onTargetLost(const DbusDictionary& share_target) {
  if (observer_ == nullptr) {
    return;
  }
  observer_->OnTargetLost(ConvertToShareTarget(share_target));
}

void NearbySharingDbusClient::onIncomingTransfer(
    const std::string& direction, const DbusDictionary& share_target,
    const DbusDictionary& transfer) {
  if (observer_ == nullptr) {
    return;
  }
  observer_->OnIncomingTransfer(direction, ConvertToShareTarget(share_target),
                                ConvertToTransfer(transfer));
}

void NearbySharingDbusClient::onTransferUpdate(
    const std::string& direction, const DbusDictionary& share_target,
    const DbusDictionary& transfer) {
  if (observer_ == nullptr) {
    return;
  }
  observer_->OnTransferUpdate(direction, ConvertToShareTarget(share_target),
                              ConvertToTransfer(transfer));
}

void NearbySharingDbusClient::onStatusChanged(const DbusDictionary& status) {
  if (observer_ == nullptr) {
    return;
  }
  observer_->OnStatusChanged(ConvertToStatus(status));
}

}  // namespace nearby::sharing::linux::app
