#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

#include <sdbus-c++/sdbus-c++.h>

#include "sharing/linux/daemon/nearby_sharing_client.h"

namespace nearby::sharing::linux::app {

using DbusDictionary = std::map<std::string, sdbus::Variant>;

struct ShareTarget {
  int64_t id = 0;
  std::string device_name;
  int32_t type = 0;
  bool is_incoming = false;
  bool is_known = false;
  std::string device_id;
  bool for_self_share = false;
  int32_t vendor_id = 0;
  bool receive_disabled = false;
};

struct Transfer {
  std::string status;
  double progress = 0;
  int64_t transferred_bytes = 0;
  int64_t total_bytes = 0;
  int64_t transfer_speed = 0;
  int64_t estimated_time_remaining = 0;
  int32_t total_attachments_count = 0;
  int32_t transferred_attachments_count = 0;
  bool is_final_status = false;
  bool is_self_share = false;
  std::string binding_id;
  std::optional<std::string> token;
  std::optional<int64_t> in_progress_attachment_id;
  std::optional<int64_t> in_progress_attachment_transferred_bytes;
  std::optional<int64_t> in_progress_attachment_total_bytes;
};

struct Status {
  bool receive_registered = false;
  bool discovery_registered = false;
  bool is_transferring = false;
  bool is_scanning = false;
  bool bluetooth_present = false;
  bool bluetooth_powered = false;
  bool lan_connected = false;
  std::vector<ShareTarget> targets;
};

class NearbySharingDbusClient
    : public sdbus::ProxyInterfaces<com::google::nearby::sharing_proxy> {
 public:
  struct Observer {
    virtual ~Observer() = default;
    virtual void OnTargetDiscovered(const ShareTarget& target) {}
    virtual void OnTargetUpdated(const ShareTarget& target) {}
    virtual void OnTargetLost(const ShareTarget& target) {}
    virtual void OnIncomingTransfer(const std::string& direction,
                                    const ShareTarget& target,
                                    const Transfer& transfer) {}
    virtual void OnTransferUpdate(const std::string& direction,
                                  const ShareTarget& target,
                                  const Transfer& transfer) {}
    virtual void OnStatusChanged(const Status& status) {}
  };

  explicit NearbySharingDbusClient(Observer* observer);
  ~NearbySharingDbusClient();

  NearbySharingDbusClient(const NearbySharingDbusClient&) = delete;
  NearbySharingDbusClient& operator=(const NearbySharingDbusClient&) = delete;

  std::tuple<bool, std::string> StartReceive();
  std::tuple<bool, std::string> StopReceive();
  std::tuple<bool, std::string> StartDiscovery();
  std::tuple<bool, std::string> StopDiscovery();
  std::tuple<bool, std::string> SendFile(int64_t share_target_id,
                                         const std::string& path);
  std::tuple<bool, std::string> Accept(int64_t share_target_id);
  std::tuple<bool, std::string> Reject(int64_t share_target_id);
  std::tuple<bool, std::string> Cancel(int64_t share_target_id);

  static ShareTarget ConvertToShareTarget(const DbusDictionary& map);
  static Transfer ConvertToTransfer(const DbusDictionary& map);
  static Status ConvertToStatus(const DbusDictionary& map);

 private:
  void onTargetDiscovered(const DbusDictionary& share_target) override;
  void onTargetUpdated(const DbusDictionary& share_target) override;
  void onTargetLost(const DbusDictionary& share_target) override;
  void onIncomingTransfer(const std::string& direction,
                          const DbusDictionary& share_target,
                          const DbusDictionary& transfer) override;
  void onTransferUpdate(const std::string& direction,
                        const DbusDictionary& share_target,
                        const DbusDictionary& transfer) override;
  void onStatusChanged(const DbusDictionary& status) override;

  Observer* observer_ = nullptr;
};

}  // namespace nearby::sharing::linux::app
