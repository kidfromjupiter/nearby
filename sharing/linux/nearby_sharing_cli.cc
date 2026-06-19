// Copyright 2026 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <signal.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include "absl/functional/any_invocable.h"
#include "absl/strings/string_view.h"
#include "absl/time/time.h"
#include "connections/implementation/flags/nearby_connections_feature_flags.h"
#include "internal/base/file_path.h"
#include "internal/flags/nearby_flags.h"
#include "proto/sharing_enums.pb.h"
#include "sharing/advertisement.h"
#include "sharing/analytics/analytics_device_settings.h"
#include "sharing/analytics/analytics_information.h"
#include "sharing/analytics/analytics_recorder.h"
#include "sharing/attachment_container.h"
#include "sharing/common/nearby_share_enums.h"
#include "sharing/file_attachment.h"
#include "sharing/linux/platform/linux_sharing_platform.h"
#include "sharing/flags/generated/nearby_sharing_feature_flags.h"
#include "sharing/nearby_sharing_service.h"
#include "sharing/nearby_sharing_service_factory.h"
#include "sharing/nearby_sharing_settings.h"
#include "sharing/share_target.h"
#include "sharing/share_target_discovered_callback.h"
#include "sharing/transfer_metadata.h"
#include "sharing/transfer_update_callback.h"
#include "sharing/proto/enums.pb.h"

namespace nearby::sharing::linux {
namespace {

std::atomic<bool> g_interrupted = false;

void HandleSignal(int signal) {
  static_cast<void>(signal);
  g_interrupted = true;
}

std::string GetHostname() {
  char hostname[256] = {};
  if (gethostname(hostname, sizeof(hostname)) == 0 && hostname[0] != '\0') {
    return std::string(hostname);
  }
  return "Linux";
}

void PrintUsage(const char* argv0) {
  std::cerr << "Usage:\n"
            << "  " << argv0
            << " receive [--name NAME] [--timeout SECONDS]\n"
            << "  " << argv0
            << " send FILE [--name NAME] [--timeout SECONDS]\n";
}

struct Options {
  enum class Mode { kReceive, kSend };

  Mode mode;
  std::string file_path;
  std::string device_name = GetHostname();
  int timeout_seconds = 120;
};

std::optional<Options> ParseArgs(int argc, char** argv) {
  if (argc < 2) {
    return std::nullopt;
  }

  Options options{.mode = Options::Mode::kReceive};
  std::string command = argv[1];
  int index = 2;
  if (command == "receive") {
    options.mode = Options::Mode::kReceive;
  } else if (command == "send") {
    options.mode = Options::Mode::kSend;
    if (index >= argc) {
      return std::nullopt;
    }
    options.file_path = argv[index++];
  } else {
    return std::nullopt;
  }

  while (index < argc) {
    std::string arg = argv[index++];
    if (arg == "--name") {
      if (index >= argc) {
        return std::nullopt;
      }
      options.device_name = argv[index++];
    } else if (arg == "--timeout") {
      if (index >= argc) {
        return std::nullopt;
      }
      options.timeout_seconds = std::atoi(argv[index++]);
      if (options.timeout_seconds < 0) {
        return std::nullopt;
      }
    } else {
      return std::nullopt;
    }
  }

  return options;
}

std::string StatusCodeToString(NearbySharingService::StatusCodes status) {
  return NearbySharingService::StatusCodeToString(status);
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

class NoOpAnalyticsRecorder final : public analytics::AnalyticsRecorder {
 public:
  void NewEstablishConnection(
      int64_t session_id,
      location::nearby::proto::sharing::EstablishConnectionStatus
          connection_status,
      const ShareTarget& share_target, int transfer_position,
      int concurrent_connections, int64_t duration_millis,
      std::optional<std::string> referrer_package) override {}
  void NewAcceptAgreements() override {}
  void NewDeclineAgreements() override {}
  void NewAddContact() override {}
  void NewRemoveContact() override {}
  void NewTapFeedback() override {}
  void NewTapHelp() override {}
  void NewLaunchDeviceContactConsent(
      location::nearby::proto::sharing::ConsentAcceptanceStatus status)
      override {}
  void NewAdvertiseDevicePresenceEnd(int64_t session_id) override {}
  void NewAdvertiseDevicePresenceStart(
      int64_t session_id, proto::DeviceVisibility visibility,
      location::nearby::proto::sharing::SessionStatus status,
      proto::DataUsage data_usage,
      std::optional<std::string> referrer_package) override {}
  void NewDescribeAttachments(const AttachmentContainer& attachments) override {}
  void NewDiscoverShareTarget(
      const ShareTarget& share_target, int64_t session_id,
      int64_t latency_since_scanning_start_millis, int64_t flow_id,
      std::optional<std::string> referrer_package,
      int64_t latency_since_send_surface_registered_millis) override {}
  void NewEnableNearbySharing(
      location::nearby::proto::sharing::NearbySharingStatus status) override {}
  void NewOpenReceivedAttachments(const AttachmentContainer& attachments,
                                  int64_t session_id) override {}
  void NewProcessReceivedAttachmentsEnd(
      int64_t session_id,
      location::nearby::proto::sharing::ProcessReceivedAttachmentsStatus status)
      override {}
  void NewReceiveAttachmentsEnd(
      int64_t session_id, int64_t received_bytes,
      location::nearby::proto::sharing::AttachmentTransmissionStatus status,
      std::optional<std::string> referrer_package) override {}
  void NewReceiveAttachmentsStart(
      int64_t session_id, const AttachmentContainer& attachments) override {}
  void NewReceiveFastInitialization(
      int64_t timeElapseSinceScreenUnlockMillis) override {}
  void NewAcceptFastInitialization() override {}
  void NewDismissFastInitialization() override {}
  void NewReceiveIntroduction(
      int64_t session_id, const ShareTarget& share_target,
      std::optional<std::string> referrer_package,
      location::nearby::proto::sharing::OSType share_target_os_type) override {}
  void NewRespondToIntroduction(
      location::nearby::proto::sharing::ResponseToIntroduction action,
      int64_t session_id) override {}
  void NewTapPrivacyNotification() override {}
  void NewDismissPrivacyNotification() override {}
  void NewScanForShareTargetsEnd(int64_t session_id) override {}
  void NewScanForShareTargetsStart(
      int64_t session_id,
      location::nearby::proto::sharing::SessionStatus status,
      analytics::AnalyticsInformation analytics_information, int64_t flow_id,
      std::optional<std::string> referrer_package) override {}
  void NewSendAttachmentsEnd(
      int64_t session_id, int64_t sent_bytes, const ShareTarget& share_target,
      location::nearby::proto::sharing::AttachmentTransmissionStatus status,
      int transfer_position, int concurrent_connections,
      int64_t duration_millis, std::optional<std::string> referrer_package,
      location::nearby::proto::sharing::ConnectionLayerStatus
          connection_layer_status,
      location::nearby::proto::sharing::OSType share_target_os_type) override {}
  void NewSendAttachmentsStart(int64_t session_id,
                               const AttachmentContainer& attachments,
                               int transfer_position,
                               int concurrent_connections,
                               bool advanced_protection_enabled,
                               bool advanced_protection_mismatch) override {}
  void NewSendFastInitialization() override {}
  void NewSendStart(int64_t session_id, int transfer_position,
                    int concurrent_connections,
                    const ShareTarget& share_target) override {}
  void NewSendIntroduction(
      ShareTargetType target_type, int64_t session_id,
      location::nearby::proto::sharing::DeviceRelationship relationship,
      location::nearby::proto::sharing::OSType share_target_os_type) override {}
  void NewSendIntroduction(
      int64_t session_id, const ShareTarget& share_target,
      int transfer_position, int concurrent_connections,
      location::nearby::proto::sharing::OSType share_target_os_type) override {}
  void NewSetVisibility(proto::DeviceVisibility src_visibility,
                        proto::DeviceVisibility dst_visibility,
                        int64_t duration_millis) override {}
  void NewDeviceSettings(analytics::AnalyticsDeviceSettings settings) override {
  }
  void NewSetDataUsage(proto::DataUsage original_preference,
                       proto::DataUsage preference) override {}
  void NewAddQuickSettingsTile() override {}
  void NewRemoveQuickSettingsTile() override {}
  void NewTapQuickSettingsTile() override {}
  void NewToggleShowNotification(
      location::nearby::proto::sharing::ShowNotificationStatus prev_status,
      location::nearby::proto::sharing::ShowNotificationStatus current_status)
      override {}
  void NewSetDeviceName(int device_name_size) override {}
  void NewRequestSettingPermissions(
      location::nearby::proto::sharing::PermissionRequestType type,
      location::nearby::proto::sharing::PermissionRequestResult result)
      override {}
  void NewInstallAPKStatus(
      location::nearby::proto::sharing::InstallAPKStatus status,
      location::nearby::proto::sharing::ApkSource source) override {}
  void NewVerifyAPKStatus(
      location::nearby::proto::sharing::VerifyAPKStatus status,
      location::nearby::proto::sharing::ApkSource source) override {}
  void NewRpcCallStatus(absl::string_view rpc_name, RpcDirection direction,
                        int error_code, absl::Duration latency) override {}
  int64_t GenerateNextId() override { return next_id_++; }

 private:
  std::atomic<int64_t> next_id_{1};
};

struct CliState {
  std::mutex mutex;
  std::condition_variable cv;
  bool done = false;
  bool send_requested = false;
  bool accept_requested = false;
  int exit_code = 1;
  std::optional<ShareTarget> selected_target;
  std::optional<int64_t> accept_target_id;
};

void PrintTransferUpdate(const ShareTarget& share_target,
                         const AttachmentContainer& attachment_container,
                         const TransferMetadata& metadata) {
  std::cout << "transfer target=\"" << share_target.device_name << "\" id="
            << share_target.id << " status="
            << TransferMetadata::StatusToString(metadata.status())
            << " progress=" << metadata.progress()
            << "% bytes=" << metadata.transferred_bytes() << "/"
            << attachment_container.GetTotalAttachmentsSize() << std::endl;
  if (metadata.token().has_value()) {
    std::cout << "confirmation token: " << *metadata.token() << std::endl;
  }
}

class CliTransferCallback final : public TransferUpdateCallback {
 public:
  CliTransferCallback(CliState& state, bool receive_mode)
      : state_(state), receive_mode_(receive_mode) {}

  void OnTransferUpdate(const ShareTarget& share_target,
                        const AttachmentContainer& attachment_container,
                        const TransferMetadata& transfer_metadata) override {
    PrintTransferUpdate(share_target, attachment_container, transfer_metadata);
    std::lock_guard<std::mutex> lock(state_.mutex);
    if (receive_mode_ &&
        transfer_metadata.status() ==
            TransferMetadata::Status::kAwaitingLocalConfirmation &&
        !state_.accept_requested) {
      state_.accept_requested = true;
      state_.accept_target_id = share_target.id;
    }
    if (TransferMetadata::IsFinalStatus(transfer_metadata.status())) {
      state_.done = true;
      state_.exit_code =
          transfer_metadata.status() == TransferMetadata::Status::kComplete ? 0
                                                                            : 1;
    }
    state_.cv.notify_all();
  }

 private:
  CliState& state_;
  bool receive_mode_;
};

class CliDiscoveryCallback final : public ShareTargetDiscoveredCallback {
 public:
  explicit CliDiscoveryCallback(CliState& state) : state_(state) {}

  void OnShareTargetDiscovered(const ShareTarget& share_target) override {
    std::cout << "discovered target=\"" << share_target.device_name
              << "\" id=" << share_target.id << std::endl;
    if (share_target.receive_disabled) {
      return;
    }
    std::lock_guard<std::mutex> lock(state_.mutex);
    if (!state_.selected_target.has_value() && !state_.send_requested) {
      state_.selected_target = share_target;
    }
    state_.cv.notify_all();
  }

  void OnShareTargetLost(const ShareTarget& share_target) override {
    std::cout << "lost target=\"" << share_target.device_name
              << "\" id=" << share_target.id << std::endl;
  }

  void OnShareTargetUpdated(const ShareTarget& share_target) override {
    std::cout << "updated target=\"" << share_target.device_name
              << "\" id=" << share_target.id << std::endl;
    OnShareTargetDiscovered(share_target);
  }

 private:
  CliState& state_;
};

class CliApp {
 public:
  explicit CliApp(const Options& options)
      : options_(options), platform_(options.device_name) {}

  int Run() {
    service_ = NearbySharingServiceFactory::GetInstance()->CreateSharingService(
        platform_, &analytics_recorder_, /*event_logger=*/nullptr,
        /*supports_file_sync=*/false);
    if (service_ == nullptr) {
      std::cerr << "failed to create NearbySharingService" << std::endl;
      return 1;
    }

    if (!ConfigureService()) {
      Shutdown();
      return 1;
    }
    int result = options_.mode == Options::Mode::kReceive ? RunReceive()
                                                          : RunSend();
    Shutdown();
    return result;
  }

 private:
  bool ConfigureService() {
    std::cout << "device name: " << options_.device_name << std::endl;
    service_->GetSettings()->SetDataUsage(proto::WIFI_ONLY_DATA_USAGE);
    service_->GetSettings()->SetDeviceName(
        options_.device_name,
        [](DeviceNameValidationResult validation_result) {
          static_cast<void>(validation_result);
    });
    auto status = WaitForStatus([&](auto callback) {
      service_->SetVisibility(proto::DEVICE_VISIBILITY_EVERYONE,
                              absl::ZeroDuration(), std::move(callback));
    });
    std::cout << "SetVisibility: " << StatusCodeToString(status) << std::endl;
    return status == NearbySharingService::StatusCodes::kOk;
  }

  int RunReceive() {
    CliState state;
    CliTransferCallback transfer_callback(state, /*receive_mode=*/true);

    auto status = WaitForStatus([&](auto callback) {
      service_->RegisterReceiveSurface(
          &transfer_callback, NearbySharingService::ReceiveSurfaceState::
                                  kForeground,
          Advertisement::BlockedVendorId::kNone, std::move(callback));
    });
    std::cout << "RegisterReceiveSurface: " << StatusCodeToString(status)
              << std::endl;
    if (status != NearbySharingService::StatusCodes::kOk) {
      return 1;
    }

    std::cout << "receiving; waiting for incoming share" << std::endl;
    int result = WaitForTransferOrActions(state);
    WaitForStatus([&](auto callback) {
      service_->UnregisterReceiveSurface(&transfer_callback,
                                         std::move(callback));
    });
    return result;
  }

  int RunSend() {
    std::error_code error;
    if (!std::filesystem::is_regular_file(options_.file_path, error)) {
      std::cerr << "not a regular file: " << options_.file_path << std::endl;
      return 1;
    }

    CliState state;
    CliTransferCallback transfer_callback(state, /*receive_mode=*/false);
    CliDiscoveryCallback discovery_callback(state);

    auto status = WaitForStatus([&](auto callback) {
      service_->RegisterSendSurface(
          &transfer_callback, &discovery_callback,
          NearbySharingService::SendSurfaceState::kForeground,
          Advertisement::BlockedVendorId::kNone,
          /*disable_wifi_hotspot=*/false, std::move(callback));
    });
    std::cout << "RegisterSendSurface: " << StatusCodeToString(status)
              << std::endl;
    if (status != NearbySharingService::StatusCodes::kOk) {
      return 1;
    }

    std::cout << "scanning; waiting for first target" << std::endl;
    int result = WaitForTransferOrActions(state);
    WaitForStatus([&](auto callback) {
      service_->UnregisterSendSurface(&transfer_callback, std::move(callback));
    });
    return result;
  }

  int WaitForTransferOrActions(CliState& state) {
    const auto start = std::chrono::steady_clock::now();
    while (!g_interrupted) {
      std::optional<int64_t> accept_target_id;
      std::optional<ShareTarget> send_target;
      {
        std::unique_lock<std::mutex> lock(state.mutex);
        if (options_.timeout_seconds == 0) {
          state.cv.wait_for(lock, std::chrono::seconds(1), [&] {
            return state.done || state.accept_target_id.has_value() ||
                   state.selected_target.has_value() || g_interrupted.load();
          });
        } else {
          auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
              std::chrono::steady_clock::now() - start);
          if (elapsed.count() >= options_.timeout_seconds) {
            std::cerr << "timed out after " << options_.timeout_seconds
                      << " seconds" << std::endl;
            return 1;
          }
          state.cv.wait_for(lock, std::chrono::seconds(1), [&] {
            return state.done || state.accept_target_id.has_value() ||
                   state.selected_target.has_value() || g_interrupted.load();
          });
        }
        if (state.done) {
          return state.exit_code;
        }
        if (state.accept_target_id.has_value()) {
          accept_target_id = state.accept_target_id;
          state.accept_target_id.reset();
        }
        if (state.selected_target.has_value() && !state.send_requested) {
          send_target = state.selected_target;
          state.send_requested = true;
        }
      }

      if (accept_target_id.has_value()) {
        std::cout << "accepting incoming share id=" << *accept_target_id
                  << std::endl;
        auto status = WaitForStatus([&](auto callback) {
          service_->Accept(*accept_target_id, std::move(callback));
        });
        std::cout << "Accept: " << StatusCodeToString(status) << std::endl;
        if (status != NearbySharingService::StatusCodes::kOk) {
          return 1;
        }
      }

      if (send_target.has_value()) {
        std::cout << "sending " << options_.file_path << " to \""
                  << send_target->device_name << "\" id=" << send_target->id
                  << std::endl;
        auto attachments = CreateFileAttachments(options_.file_path);
        auto status = WaitForStatus([&](auto callback) {
          service_->SendAttachments(send_target->id, std::move(attachments),
                                    std::move(callback));
        });
        std::cout << "SendAttachments: " << StatusCodeToString(status)
                  << std::endl;
        if (status != NearbySharingService::StatusCodes::kOk) {
          return 1;
        }
      }
    }

    std::cerr << "interrupted" << std::endl;
    return 1;
  }

  void Shutdown() {
    if (service_ == nullptr) {
      return;
    }
    auto status = WaitForStatus(
        [&](auto callback) { service_->Shutdown(std::move(callback)); });
    std::cout << "Shutdown: " << StatusCodeToString(status) << std::endl;
  }

  Options options_;
  LinuxSharingPlatform platform_;
  NoOpAnalyticsRecorder analytics_recorder_;
  NearbySharingService* service_ = nullptr;
};

}  // namespace
}  // namespace nearby::sharing::linux

int main(int argc, char** argv) {
  signal(SIGINT, nearby::sharing::linux::HandleSignal);
  signal(SIGTERM, nearby::sharing::linux::HandleSignal);
  nearby::NearbyFlags::GetInstance().OverrideBoolFlagValue(
      nearby::sharing::config_package_nearby::nearby_sharing_feature::
          kEnableBleForTransfer,
      true);
  nearby::NearbyFlags::GetInstance().OverrideBoolFlagValue(
      nearby::connections::config_package_nearby::nearby_connections_feature::
          kEnableBleL2cap,
      true);
  nearby::NearbyFlags::GetInstance().OverrideBoolFlagValue(
      nearby::connections::config_package_nearby::nearby_connections_feature::
          kRefactorBleL2cap,
      true);

  std::optional<nearby::sharing::linux::Options> options =
      nearby::sharing::linux::ParseArgs(argc, argv);
  if (!options.has_value()) {
    nearby::sharing::linux::PrintUsage(argv[0]);
    return 2;
  }

  if (options->mode == nearby::sharing::linux::Options::Mode::kSend) {
    std::error_code error;
    if (!std::filesystem::is_regular_file(options->file_path, error)) {
      std::cerr << "not a regular file: " << options->file_path << std::endl;
      return 1;
    }
  }

  nearby::sharing::linux::CliApp app(*options);
  return app.Run();
}
