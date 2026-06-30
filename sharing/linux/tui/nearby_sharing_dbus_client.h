#include "sharing/linux/daemon/nearby_sharing_client.h"

struct ShareTarget {
    int64_t id;
    std::string device_name;
    int32_t type;
    bool is_incoming;
    bool is_known;
    std::string device_id;
    bool for_self_share;
    int32_t vendor_id;
    bool receive_disabled;
};

struct Transfer {
    std::string status;
    int32_t progress;
    int64_t transferred_bytes;
    int64_t total_bytes;
    int64_t transfer_speed;
    int64_t estimated_time_remaining;
    int32_t total_attachments_count;
    int32_t transferred_attachments_count;
    bool is_final_status;
    bool is_self_share;
    std::string binding_id;
    std::optional<std::string> token;
    std::optional<int64_t> in_progress_attachment_id;
    std::optional<int64_t> in_progress_attachment_transferred_bytes;
    std::optional<int64_t> in_progress_attachment_total_bytes;
};

struct Status {
    bool receive_registered;
    bool discovery_registered;
    bool is_transferring;
    bool is_scanning;
    bool bluetooth_present;
    bool bluetooth_powered;
    bool lan_connected;
    
    // aa{sv} maps to a vector of ShareTarget structs
    std::vector<ShareTarget> targets;
};


class NearbySharingService
    : public sdbus::ProxyInterfaces<com::google::nearby::sharing_proxy> {
 public:
  NearbySharingService(sdbus::ServiceName dest, sdbus::ObjectPath objectPath)
      : ProxyInterfaces(std::move(dest), std::move(objectPath)) {
    registerProxy();
  }
  ~NearbySharingService() { unregisterProxy(); }

  void onTargetDiscovered(
      const std::map<std::string, sdbus::Variant>& share_target) override;
  void onTargetUpdated(
      const std::map<std::string, sdbus::Variant>& share_target) override;
  void onTargetLost(
      const std::map<std::string, sdbus::Variant>& share_target) override;
  void onIncomingTransfer(
      const std::string& direction,
      const std::map<std::string, sdbus::Variant>& share_target,
      const std::map<std::string, sdbus::Variant>& transfer) override;
  void onTransferUpdate(
      const std::string& direction,
      const std::map<std::string, sdbus::Variant>& share_target,
      const std::map<std::string, sdbus::Variant>& transfer) override;
  void onStatusChanged(
      const std::map<std::string, sdbus::Variant>& status) override;

 private:
  Status status_;
  std::map<int, ShareTarget> targets_;
  Transfer outgoing_transfer_;
  Transfer incoming_transfer_;
};
