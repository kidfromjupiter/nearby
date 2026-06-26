#include "sharing/linux/tui/nearby_sharing_dbus_client.h"

// Helper to extract a value from the map with a fallback if missing or type
// mismatches
template <typename T>
T get_field(const std::map<std::string, sdbus::Variant>& map,
            const std::string& key, T default_value = T{}) {
  auto it = map.find(key);
  if (it != map.end()) {
    try {
      return it->second.get<T>();
    } catch (const sdbus::Error& e) {
      // Log type mismatch error if necessary
    }
  }
  return default_value;
}

// Helper for optional fields
template <typename T>
std::optional<T> get_optional_field(
    const std::map<std::string, sdbus::Variant>& map, const std::string& key) {
  auto it = map.find(key);
  if (it != map.end()) {
    try {
      return it->second.get<T>();
    } catch (const sdbus::Error& e) {
      // Log type mismatch error if necessary
    }
  }
  return std::nullopt;
}

// Converter: Map -> ShareTarget
ShareTarget convertToShareTarget(
    const std::map<std::string, sdbus::Variant>& map) {
  ShareTarget target;
  target.id = get_field<int64_t>(map, "id");
  target.device_name = get_field<std::string>(map, "device_name");
  target.type = get_field<int32_t>(map, "type");
  target.is_incoming = get_field<bool>(map, "is_incoming");
  target.is_known = get_field<bool>(map, "is_known");
  target.device_id = get_field<std::string>(map, "device_id");
  target.for_self_share = get_field<bool>(map, "for_self_share");
  target.vendor_id = get_field<int32_t>(map, "vendor_id");
  target.receive_disabled = get_field<bool>(map, "receive_disabled");
  return target;
}

// Converter: Map -> Transfer
Transfer convertToTransfer(const std::map<std::string, sdbus::Variant>& map) {
  Transfer transfer;
  transfer.status = get_field<std::string>(map, "status");
  transfer.progress = get_field<int32_t>(map, "progress");
  transfer.transferred_bytes = get_field<int64_t>(map, "transferred_bytes");
  transfer.total_bytes = get_field<int64_t>(map, "total_bytes");
  transfer.transfer_speed = get_field<int64_t>(map, "transfer_speed");
  transfer.estimated_time_remaining =
      get_field<int64_t>(map, "estimated_time_remaining");
  transfer.total_attachments_count =
      get_field<int32_t>(map, "total_attachments_count");
  transfer.transferred_attachments_count =
      get_field<int32_t>(map, "transferred_attachments_count");
  transfer.is_final_status = get_field<bool>(map, "is_final_status");
  transfer.is_self_share = get_field<bool>(map, "is_self_share");
  transfer.binding_id = get_field<std::string>(map, "binding_id");

  // Optional fields
  transfer.token = get_optional_field<std::string>(map, "token");
  transfer.in_progress_attachment_id =
      get_optional_field<int64_t>(map, "in_progress_attachment_id");
  transfer.in_progress_attachment_transferred_bytes =
      get_optional_field<int64_t>(map,
                                  "in_progress_attachment_transferred_bytes");
  transfer.in_progress_attachment_total_bytes =
      get_optional_field<int64_t>(map, "in_progress_attachment_total_bytes");
  return transfer;
}

// Converter: Map -> Status
Status convertToStatus(const std::map<std::string, sdbus::Variant>& map) {
  Status status;
  status.receive_registered = get_field<bool>(map, "receive_registered");
  status.discovery_registered = get_field<bool>(map, "discovery_registered");
  status.is_transferring = get_field<bool>(map, "is_transferring");
  status.is_scanning = get_field<bool>(map, "is_scanning");
  status.bluetooth_present = get_field<bool>(map, "bluetooth_present");
  status.bluetooth_powered = get_field<bool>(map, "bluetooth_powered");
  status.lan_connected = get_field<bool>(map, "lan_connected");

  // Unpack aa{sv} (vector of maps) into vector of ShareTarget structs
  auto it = map.find("targets");
  if (it != map.end()) {
    try {
      auto raw_targets =
          it->second.get<std::vector<std::map<std::string, sdbus::Variant>>>();
      for (const auto& target_map : raw_targets) {
        status.targets.push_back(convertToShareTarget(target_map));
      }
    } catch (const sdbus::Error& e) {
      // Handle type mismatch for targets array
    }
  }
  return status;
}

void NearbySharingService::onTargetDiscovered(
    const std::map<std::string, sdbus::Variant>& share_target) {
  auto target = convertToShareTarget(share_target);
  targets_[target.id] = target;
};

void NearbySharingService::onTargetUpdated(
    const std::map<std::string, sdbus::Variant>& share_target) {
  auto target = convertToShareTarget(share_target);
  targets_[target.id] = target;
};

void NearbySharingService::onTargetLost(
    const std::map<std::string, sdbus::Variant>& share_target) {
  auto target = convertToShareTarget(share_target);
  if (targets_.find(target.id) != targets_.end()) {
    targets_.erase(target.id);
  }
};

void NearbySharingService::onIncomingTransfer(
    const std::string& direction,
    const std::map<std::string, sdbus::Variant>& share_target,
    const std::map<std::string, sdbus::Variant>& transfer) {
  auto target = convertToShareTarget(share_target);
  auto incoming_transfer = convertToTransfer(transfer);

  incoming_transfer_ = incoming_transfer;
};

void NearbySharingService::onTransferUpdate(
    const std::string& direction,
    const std::map<std::string, sdbus::Variant>& share_target,
    const std::map<std::string, sdbus::Variant>& transfer) {};

void NearbySharingService::onStatusChanged(
    const std::map<std::string, sdbus::Variant>& share_target) {};
