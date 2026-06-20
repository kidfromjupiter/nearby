#include "sharing/linux/daemon/daemon_service.h"

#include <gtest/gtest.h>

#include <functional>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <cstdio>
#include <vector>

#include "absl/functional/any_invocable.h"
#include "absl/time/time.h"
#include "nlohmann/json.hpp"
#include "sharing/attachment_container.h"
#include "sharing/nearby_sharing_service.h"
#include "sharing/share_target.h"
#include "sharing/share_target_discovered_callback.h"
#include "sharing/transfer_metadata.h"
#include "sharing/transfer_metadata_builder.h"
#include "sharing/transfer_update_callback.h"

namespace nearby::sharing::linux {
namespace {

ShareTarget MakeTarget(int64_t id, std::string name = "Pixel") {
  ShareTarget target;
  target.id = id;
  target.device_name = std::move(name);
  return target;
}

std::unique_ptr<AttachmentContainer> MakeAttachments() {
  AttachmentContainer::Builder builder;
  return builder.Build();
}

TransferMetadata MakeMetadata(TransferMetadata::Status status) {
  return TransferMetadataBuilder()
      .set_status(status)
      .set_progress(status == TransferMetadata::Status::kComplete ? 100 : 25)
      .set_total_attachments_count(1)
      .build();
}

class FakeDaemonNearbySharingService final : public NearbySharingService {
 public:
  void AddObserver(Observer* observer) override { static_cast<void>(observer); }
  void RemoveObserver(Observer* observer) override {
    static_cast<void>(observer);
  }
  void Shutdown(std::function<void(StatusCodes)> callback) override {
    shutdown_called = true;
    callback(StatusCodes::kOk);
  }
  void RegisterSendSurface(
      TransferUpdateCallback* transfer_callback,
      ShareTargetDiscoveredCallback* discovery_callback, SendSurfaceState state,
      Advertisement::BlockedVendorId blocked_vendor_id,
      bool disable_wifi_hotspot,
      absl::AnyInvocable<void(StatusCodes)> callback) override {
    static_cast<void>(blocked_vendor_id);
    static_cast<void>(disable_wifi_hotspot);
    send_transfer_callback = transfer_callback;
    this->discovery_callback = discovery_callback;
    send_state = state;
    callback(StatusCodes::kOk);
  }
  void UnregisterSendSurface(
      TransferUpdateCallback* transfer_callback,
      absl::AnyInvocable<void(StatusCodes)> callback) override {
    if (send_transfer_callback == transfer_callback) {
      send_transfer_callback = nullptr;
      discovery_callback = nullptr;
    }
    callback(StatusCodes::kOk);
  }
  void RegisterReceiveSurface(
      TransferUpdateCallback* transfer_callback, ReceiveSurfaceState state,
      Advertisement::BlockedVendorId vendor_id,
      absl::AnyInvocable<void(StatusCodes)> callback) override {
    static_cast<void>(vendor_id);
    receive_transfer_callback = transfer_callback;
    receive_state = state;
    callback(StatusCodes::kOk);
  }
  void UnregisterReceiveSurface(
      TransferUpdateCallback* transfer_callback,
      absl::AnyInvocable<void(StatusCodes)> callback) override {
    if (receive_transfer_callback == transfer_callback) {
      receive_transfer_callback = nullptr;
    }
    callback(StatusCodes::kOk);
  }
  void ClearForegroundReceiveSurfaces(
      absl::AnyInvocable<void(StatusCodes)> callback) override {
    callback(StatusCodes::kOk);
  }
  bool IsTransferring() const override { return false; }
  bool IsScanning() const override { return discovery_callback != nullptr; }
  bool IsBluetoothPresent() const override { return true; }
  bool IsBluetoothPowered() const override { return true; }
  bool IsExtendedAdvertisingSupported() const override { return true; }
  bool IsLanConnected() const override { return true; }
  std::string GetQrCodeUrl() const override { return ""; }
  void SendAttachments(
      int64_t share_target_id,
      std::unique_ptr<AttachmentContainer> attachment_container,
      std::function<void(StatusCodes)> callback) override {
    last_send_target_id = share_target_id;
    last_attachment_count = attachment_container == nullptr
                                ? 0
                                : attachment_container->GetAttachmentCount();
    callback(StatusCodes::kOk);
  }
  void Accept(int64_t share_target_id,
              std::function<void(StatusCodes)> callback) override {
    last_accept_target_id = share_target_id;
    callback(StatusCodes::kOk);
  }
  void Reject(int64_t share_target_id,
              std::function<void(StatusCodes)> callback) override {
    last_reject_target_id = share_target_id;
    callback(StatusCodes::kOk);
  }
  void Cancel(int64_t share_target_id,
              std::function<void(StatusCodes)> callback) override {
    last_cancel_target_id = share_target_id;
    callback(StatusCodes::kOk);
  }
  void InitiatePairing(
      int64_t share_target_id,
      service::proto::BindingRequest::Type binding_type,
      absl::AnyInvocable<void(StatusCodes status_codes) &&> callback) override {
    static_cast<void>(share_target_id);
    static_cast<void>(binding_type);
    std::move(callback)(StatusCodes::kOk);
  }
  void SetVisibility(
      proto::DeviceVisibility visibility, absl::Duration expiration,
      absl::AnyInvocable<void(StatusCodes status_code) &&> callback) override {
    static_cast<void>(visibility);
    static_cast<void>(expiration);
    std::move(callback)(StatusCodes::kOk);
  }
  std::string Dump() const override { return ""; }
  void UpdateFilePathsInProgress(bool update_file_paths) override {
    static_cast<void>(update_file_paths);
  }
  NearbyShareSettings* GetSettings() override { return nullptr; }
  NearbyShareCertificateManager* GetCertificateManager() override {
    return nullptr;
  }
  AccountManager* GetAccountManager() override { return nullptr; }
  Clock& GetClock() override { throw std::logic_error("unused"); }
  void SetAlternateServiceUuidForDiscovery(uint16_t uuid) override {
    static_cast<void>(uuid);
  }
  SyncManager& sync_manager() override { throw std::logic_error("unused"); }
  OutgoingTargetsManager& outgoing_targets_manager() override {
    throw std::logic_error("unused");
  }
  void UpdateBackupSavePath(
      absl::string_view binding_id, absl::string_view save_path,
      absl::AnyInvocable<void(NearbySharingService::StatusCodes)> callback)
      override {
    static_cast<void>(binding_id);
    static_cast<void>(save_path);
    callback(StatusCodes::kOk);
  }

  void FireShareTargetDiscovered(const ShareTarget& target) {
    discovery_callback->OnShareTargetDiscovered(target);
  }
  void FireShareTargetLost(const ShareTarget& target) {
    discovery_callback->OnShareTargetLost(target);
  }
  void FireReceiveTransferUpdate(const ShareTarget& target,
                                 const AttachmentContainer& attachments,
                                 const TransferMetadata& metadata) {
    receive_transfer_callback->OnTransferUpdate(target, attachments, metadata);
  }

  TransferUpdateCallback* send_transfer_callback = nullptr;
  TransferUpdateCallback* receive_transfer_callback = nullptr;
  ShareTargetDiscoveredCallback* discovery_callback = nullptr;
  SendSurfaceState send_state = SendSurfaceState::kUnknown;
  ReceiveSurfaceState receive_state = ReceiveSurfaceState::kUnknown;
  bool shutdown_called = false;
  int64_t last_send_target_id = 0;
  size_t last_attachment_count = 0;
  int64_t last_accept_target_id = 0;
  int64_t last_reject_target_id = 0;
  int64_t last_cancel_target_id = 0;
};

class DaemonServiceTest : public ::testing::Test {
 protected:
  DaemonServiceTest()
      : daemon_(service_, [this](const nlohmann::json& event) {
          events_.push_back(event);
        }) {}

  FakeDaemonNearbySharingService service_;
  std::vector<nlohmann::json> events_;
  DaemonService daemon_;
};

TEST_F(DaemonServiceTest, StartDiscoveryPublishesDiscoveredTargets) {
  nlohmann::json result =
      daemon_.HandleCommand({{"command", "start_discovery"}});
  ASSERT_TRUE(result["ok"].get<bool>());

  service_.FireShareTargetDiscovered(MakeTarget(42));

  ASSERT_EQ(events_.size(), 1u);
  EXPECT_EQ(events_[0]["event"], "target_discovered");
  EXPECT_EQ(events_[0]["share_target"]["id"], 42);
  EXPECT_EQ(events_[0]["share_target"]["device_name"], "Pixel");

  nlohmann::json status = daemon_.HandleCommand({{"command", "status"}});
  ASSERT_TRUE(status["ok"].get<bool>());
  ASSERT_EQ(status["targets"].size(), 1u);
  EXPECT_EQ(status["targets"][0]["id"], 42);
}

TEST_F(DaemonServiceTest, TargetLostRemovesTargetFromStatus) {
  ASSERT_TRUE(
      daemon_.HandleCommand({{"command", "start_discovery"}})["ok"].get<bool>());
  ShareTarget target = MakeTarget(7);
  service_.FireShareTargetDiscovered(target);
  service_.FireShareTargetLost(target);

  ASSERT_EQ(events_.size(), 2u);
  EXPECT_EQ(events_[1]["event"], "target_lost");

  nlohmann::json status = daemon_.HandleCommand({{"command", "status"}});
  ASSERT_TRUE(status["ok"].get<bool>());
  EXPECT_TRUE(status["targets"].empty());
}

TEST_F(DaemonServiceTest, ReceiveAwaitingConfirmationPublishesIncomingTransfer) {
  ASSERT_TRUE(
      daemon_.HandleCommand({{"command", "start_receive"}})["ok"].get<bool>());

  auto attachments = MakeAttachments();
  service_.FireReceiveTransferUpdate(
      MakeTarget(9), *attachments,
      MakeMetadata(TransferMetadata::Status::kAwaitingLocalConfirmation));

  ASSERT_EQ(events_.size(), 1u);
  EXPECT_EQ(events_[0]["event"], "incoming_transfer");
  EXPECT_EQ(events_[0]["direction"], "receive");
  EXPECT_EQ(events_[0]["share_target"]["id"], 9);
  EXPECT_EQ(events_[0]["transfer"]["status"], "kAwaitingLocalConfirmation");
}

TEST_F(DaemonServiceTest, AcceptRejectAndCancelReturnCommandResults) {
  EXPECT_TRUE(daemon_.HandleCommand({{"command", "accept"},
                                    {"share_target_id", 1}})["ok"]
                  .get<bool>());
  EXPECT_EQ(service_.last_accept_target_id, 1);
  EXPECT_TRUE(daemon_.HandleCommand({{"command", "reject"},
                                    {"share_target_id", 1}})["ok"]
                  .get<bool>());
  EXPECT_EQ(service_.last_reject_target_id, 1);
  EXPECT_TRUE(daemon_.HandleCommand({{"command", "cancel"},
                                    {"share_target_id", 1}})["ok"]
                  .get<bool>());
  EXPECT_EQ(service_.last_cancel_target_id, 1);
}

TEST_F(DaemonServiceTest, SendFileRejectsUnknownTarget) {
  nlohmann::json result = daemon_.HandleCommand(
      {{"command", "send_file"}, {"share_target_id", 404}, {"path", "/tmp/x"}});
  EXPECT_FALSE(result["ok"].get<bool>());
}

TEST_F(DaemonServiceTest, SendFileToKnownTargetReturnsOkForRegularFile) {
  ASSERT_TRUE(
      daemon_.HandleCommand({{"command", "start_discovery"}})["ok"].get<bool>());
  service_.FireShareTargetDiscovered(MakeTarget(12));

  const std::string path = "/tmp/nearby_daemon_service_test_file";
  {
    std::ofstream file(path);
    file << "hello";
  }

  nlohmann::json result = daemon_.HandleCommand(
      {{"command", "send_file"}, {"share_target_id", 12}, {"path", path}});
  EXPECT_TRUE(result["ok"].get<bool>()) << result.dump();
  EXPECT_EQ(service_.last_send_target_id, 12);
  EXPECT_EQ(service_.last_attachment_count, 1u);
  std::remove(path.c_str());
}

TEST_F(DaemonServiceTest, MalformedCommandReturnsError) {
  nlohmann::json result = daemon_.HandleCommand({{"path", "/tmp/x"}});
  EXPECT_FALSE(result["ok"].get<bool>());
}

}  // namespace
}  // namespace nearby::sharing::linux
