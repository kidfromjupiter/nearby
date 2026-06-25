#ifndef SHARING_LINUX_DAEMON_NEARBY_SHARING_DBUS_SERVICE_H_
#define SHARING_LINUX_DAEMON_NEARBY_SHARING_DBUS_SERVICE_H_

#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <tuple>
#include <unordered_map>

#include <sdbus-c++/sdbus-c++.h>

#include "absl/synchronization/mutex.h"
#include "sharing/attachment_container.h"
#include "sharing/linux/daemon/nearby_sharing_server.h"
#include "sharing/nearby_sharing_service.h"
#include "sharing/share_target.h"
#include "sharing/share_target_discovered_callback.h"
#include "sharing/transfer_metadata.h"
#include "sharing/transfer_update_callback.h"

namespace nearby::sharing::linux {

using DbusDictionary = std::map<std::string, sdbus::Variant>;

DbusDictionary ShareTargetToDbus(const ShareTarget& share_target);
DbusDictionary TransferMetadataToDbus(
    const TransferMetadata& transfer_metadata,
    const AttachmentContainer& attachment_container);

class NearbySharingDbusService final
    : public com::google::nearby::sharing_adaptor,
      public ShareTargetDiscoveredCallback,
      public NearbySharingService::Observer {
 public:
  using QuitCallback = std::function<void()>;

  NearbySharingDbusService(sdbus::IObject& object,
                           NearbySharingService& service,
                           QuitCallback quit_callback);
  ~NearbySharingDbusService() override;

  NearbySharingDbusService(const NearbySharingDbusService&) = delete;
  NearbySharingDbusService& operator=(const NearbySharingDbusService&) = delete;

  void EmitStatusChanged();
  void ShutdownService();

  void OnShareTargetDiscovered(const ShareTarget& share_target) override;
  void OnShareTargetLost(const ShareTarget& share_target) override;
  void OnShareTargetUpdated(const ShareTarget& share_target) override;

  void OnHighVisibilityChanged(bool in_high_visibility) override;
  void OnBluetoothStatusChanged(AdapterState state) override;
  void OnLanStatusChanged(AdapterState state) override;
  void OnIrrecoverableHardwareErrorReported() override;

 private:
  class TransferCallback final : public TransferUpdateCallback {
   public:
    TransferCallback(NearbySharingDbusService& service, bool receive_mode);

    void OnTransferUpdate(const ShareTarget& share_target,
                          const AttachmentContainer& attachment_container,
                          const TransferMetadata& transfer_metadata) override;

   private:
    NearbySharingDbusService& service_;
    bool receive_mode_;
  };

  std::tuple<bool, std::string> StartReceive() override;
  std::tuple<bool, std::string> StopReceive() override;
  std::tuple<bool, std::string> StartDiscovery() override;
  std::tuple<bool, std::string> StopDiscovery() override;
  std::tuple<bool, std::string> SendFile(const int64_t& share_target_id,
                                         const std::string& path) override;
  std::tuple<bool, std::string> Accept(const int64_t& share_target_id) override;
  std::tuple<bool, std::string> Reject(const int64_t& share_target_id) override;
  std::tuple<bool, std::string> Cancel(const int64_t& share_target_id) override;
  std::tuple<bool, std::string> Shutdown() override;

  std::tuple<bool, std::string> InvokeStatusCommand(
      std::function<
          void(std::function<void(NearbySharingService::StatusCodes)>)>
          invoker);
  DbusDictionary StatusToDbus() const;
  void OnTransferUpdate(bool receive_mode, const ShareTarget& share_target,
                        const AttachmentContainer& attachment_container,
                        const TransferMetadata& transfer_metadata);

  NearbySharingService& service_;
  QuitCallback quit_callback_;
  TransferCallback send_transfer_callback_;
  TransferCallback receive_transfer_callback_;
  mutable absl::Mutex lock_;
  std::unordered_map<int64_t, ShareTarget> targets_;
  bool receive_registered_ = false;
  bool discovery_registered_ = false;
  bool shutdown_ = false;
};

}  // namespace nearby::sharing::linux

#endif  // SHARING_LINUX_DAEMON_NEARBY_SHARING_DBUS_SERVICE_H_
