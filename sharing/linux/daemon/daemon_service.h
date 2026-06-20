#ifndef SHARING_LINUX_DAEMON_DAEMON_SERVICE_H_
#define SHARING_LINUX_DAEMON_DAEMON_SERVICE_H_

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>

#include "absl/synchronization/mutex.h"
#include "nlohmann/json.hpp"
#include "sharing/attachment_container.h"
#include "sharing/nearby_sharing_service.h"
#include "sharing/share_target.h"
#include "sharing/share_target_discovered_callback.h"
#include "sharing/transfer_metadata.h"
#include "sharing/transfer_update_callback.h"

namespace nearby::sharing::linux {

// Owns the daemon-facing state and translates JSON IPC commands into
// NearbySharingService operations. This class does not own `service`; callers
// must keep the service alive for the lifetime of DaemonService.
class DaemonService {
 public:
  // Receives asynchronous daemon events, such as discovered targets and
  // transfer updates, serialized as JSON objects.
  using EventSink = std::function<void(const nlohmann::json& event)>;

  // Creates a daemon adapter around `service` and publishes asynchronous events
  // to `event_sink`.
  DaemonService(NearbySharingService& service, EventSink event_sink);

  // Stops registered surfaces and shuts down the wrapped Nearby service.
  ~DaemonService();

  DaemonService(const DaemonService&) = delete;
  DaemonService& operator=(const DaemonService&) = delete;

  // Handles one JSON IPC command and returns a command_result JSON object.
  // The request must contain a string `command` field.
  nlohmann::json HandleCommand(const nlohmann::json& request);

  // Idempotently unregisters active surfaces, clears daemon state, and shuts
  // down the wrapped Nearby service.
  void Shutdown();

 private:
  // Receives send or receive transfer updates from NearbySharingService and
  // forwards them back into DaemonService with direction context.
  class TransferCallback final : public TransferUpdateCallback {
   public:
    // Creates a transfer callback. `receive_mode` distinguishes receive events
    // from send events when serializing daemon events.
    explicit TransferCallback(DaemonService& daemon, bool receive_mode)
        : daemon_(daemon), receive_mode_(receive_mode) {}

    // Forwards a Nearby transfer update to the daemon event pipeline.
    void OnTransferUpdate(const ShareTarget& share_target,
                          const AttachmentContainer& attachment_container,
                          const TransferMetadata& transfer_metadata) override;

   private:
    DaemonService& daemon_;
    bool receive_mode_;
  };

  // Receives discovered target lifecycle events from NearbySharingService and
  // forwards them back into DaemonService.
  class DiscoveryCallback final : public ShareTargetDiscoveredCallback {
   public:
    // Creates a discovery callback bound to `daemon`.
    explicit DiscoveryCallback(DaemonService& daemon) : daemon_(daemon) {}

    // Records and emits a newly discovered share target.
    void OnShareTargetDiscovered(const ShareTarget& share_target) override;

    // Removes and emits a lost share target.
    void OnShareTargetLost(const ShareTarget& share_target) override;

    // Updates and emits an existing share target.
    void OnShareTargetUpdated(const ShareTarget& share_target) override;

   private:
    DaemonService& daemon_;
  };

  // Registers the foreground receive surface used for incoming transfers.
  nlohmann::json StartReceive();

  // Unregisters the foreground receive surface if it is active.
  nlohmann::json StopReceive();

  // Registers the foreground send surface used for discovery and outgoing
  // transfer updates.
  nlohmann::json StartDiscovery();

  // Unregisters the foreground send surface and clears discovered targets.
  nlohmann::json StopDiscovery();

  // Sends the regular file named by request field `path` to request field
  // `share_target_id`.
  nlohmann::json SendFile(const nlohmann::json& request);

  // Accepts an incoming share identified by request field `share_target_id`.
  nlohmann::json Accept(const nlohmann::json& request);

  // Rejects an incoming share identified by request field `share_target_id`.
  nlohmann::json Reject(const nlohmann::json& request);

  // Cancels a transfer identified by request field `share_target_id`.
  nlohmann::json Cancel(const nlohmann::json& request);

  // Returns current daemon/service state, including registered surfaces and
  // discovered targets.
  nlohmann::json Status();

  // Updates active transfer state and emits either `incoming_transfer` or
  // `transfer_update`.
  void OnTransferUpdate(bool receive_mode, const ShareTarget& share_target,
                        const AttachmentContainer& attachment_container,
                        const TransferMetadata& transfer_metadata);

  // Stores and emits a newly discovered target.
  void OnShareTargetDiscovered(const ShareTarget& share_target);

  // Stores and emits an updated target.
  void OnShareTargetUpdated(const ShareTarget& share_target);

  // Removes and emits a lost target.
  void OnShareTargetLost(const ShareTarget& share_target);

  // Runs an asynchronous Nearby operation synchronously and converts its status
  // callback into a command_result JSON object.
  nlohmann::json InvokeStatusCommand(
      const std::string& command,
      std::function<void(std::function<void(NearbySharingService::StatusCodes)>)>
          invoker);

  // Extracts request field `share_target_id`; writes a command_result-style
  // error object and returns nullopt when the field is absent or not integral.
  std::optional<int64_t> GetRequiredTargetId(const nlohmann::json& request,
                                             nlohmann::json& error) const;

  // Publishes an asynchronous event through the configured EventSink.
  void Emit(nlohmann::json event);

  NearbySharingService& service_;
  EventSink event_sink_;
  TransferCallback send_transfer_callback_;
  TransferCallback receive_transfer_callback_;
  DiscoveryCallback discovery_callback_;
  mutable absl::Mutex lock_;
  std::unordered_map<int64_t, ShareTarget> targets_;
  std::unordered_map<int64_t, ShareTarget> active_transfers_;
  bool receive_registered_ = false;
  bool discovery_registered_ = false;
  bool shutdown_ = false;
};

// Serializes the subset of ShareTarget fields exposed through daemon IPC.
nlohmann::json ShareTargetToJson(const ShareTarget& share_target);

// Serializes transfer progress and attachment totals exposed through daemon
// IPC.
nlohmann::json TransferMetadataToJson(
    const TransferMetadata& transfer_metadata,
    const AttachmentContainer& attachment_container);

}  // namespace nearby::sharing::linux

#endif  // SHARING_LINUX_DAEMON_DAEMON_SERVICE_H_
