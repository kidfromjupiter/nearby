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

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "absl/status/status.h"
#include "absl/synchronization/notification.h"
#include "internal/platform/system_clock.h"
#include "internal/platform/single_thread_executor.h"
#include "presence/broadcast_request.h"
#include "presence/data_element.h"
#include "presence/implementation/broadcast_manager.h"
#include "presence/implementation/credential_manager.h"
#include "presence/implementation/mediums/mediums.h"
#include "presence/power_mode.h"

namespace {

absl::Notification g_shutdown;

void QuitHandler(int, siginfo_t*, void*) {
  if (!g_shutdown.HasBeenNotified()) g_shutdown.Notify();
}

void PrintUsage(const char* program) {
  std::cerr
      << "Usage: " << program << " [--name <device_name>] [--tx_power <int>]\n"
      << "Example:\n"
      << "  " << program << " --name \"Linux Presence Demo\" --tx_power 20\n";
}

// For a public-identity broadcast demo, credentials are never requested.
class DemoCredentialManager : public nearby::presence::CredentialManager {
 public:
  void GenerateCredentials(
      const nearby::internal::DeviceIdentityMetaData&,
      absl::string_view,
      const std::vector<nearby::internal::IdentityType>&,
      int,
      int,
      nearby::presence::GenerateCredentialsResultCallback callback) override {
    callback.credentials_generated_cb(
        absl::UnimplementedError("Not used in public broadcast demo"));
  }

  void UpdateRemotePublicCredentials(
      absl::string_view,
      absl::string_view,
      const std::vector<nearby::internal::SharedCredential>&,
      nearby::presence::UpdateRemotePublicCredentialsCallback callback)
      override {
    callback.credentials_updated_cb(
        absl::UnimplementedError("Not used in public broadcast demo"));
  }

  void UpdateLocalCredential(
      const nearby::presence::CredentialSelector&,
      nearby::internal::LocalCredential,
      nearby::presence::SaveCredentialsResultCallback callback) override {
    callback.credentials_saved_cb(
        absl::UnimplementedError("Not used in public broadcast demo"));
  }

  void GetLocalCredentials(
      const nearby::presence::CredentialSelector&,
      nearby::presence::GetLocalCredentialsResultCallback callback) override {
    callback.credentials_fetched_cb(
        absl::NotFoundError("No local credentials in demo manager"));
  }

  void GetPublicCredentials(
      const nearby::presence::CredentialSelector&,
      nearby::presence::PublicCredentialType,
      nearby::presence::GetPublicCredentialsResultCallback callback) override {
    callback.credentials_fetched_cb(
        absl::NotFoundError("No public credentials in demo manager"));
  }

  nearby::presence::SubscriberId SubscribeForPublicCredentials(
      const nearby::presence::CredentialSelector&,
      nearby::presence::PublicCredentialType,
      nearby::presence::GetPublicCredentialsResultCallback callback) override {
    callback.credentials_fetched_cb(
        absl::NotFoundError("No subscriptions in demo manager"));
    return 0;
  }

  void UnsubscribeFromPublicCredentials(nearby::presence::SubscriberId) override {
  }

  std::string DecryptDeviceIdentityMetaData(
      absl::string_view,
      absl::string_view,
      absl::string_view) override {
    return "";
  }

  void SetDeviceIdentityMetaData(
      const nearby::internal::DeviceIdentityMetaData& device_identity_metadata,
      bool,
      absl::string_view,
      const std::vector<nearby::internal::IdentityType>&,
      int,
      int,
      nearby::presence::GenerateCredentialsResultCallback) override {
    metadata_ = device_identity_metadata;
  }

  nearby::internal::DeviceIdentityMetaData GetDeviceIdentityMetaData()
      override {
    return metadata_;
  }

 private:
  nearby::internal::DeviceIdentityMetaData metadata_;
};

}  // namespace

int main(int argc, char** argv) {
  // Force-link the platform SystemClock implementation from static archives.
  (void)nearby::SystemClock::ElapsedRealtime();

  std::string device_name = "Nearby Presence Demo";
  int tx_power = 20;

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if ((arg == "-h") || (arg == "--help")) {
      PrintUsage(argv[0]);
      return 0;
    }
    if (arg == "--name") {
      if (i + 1 >= argc) {
        std::cerr << "Missing value for --name\n";
        return 2;
      }
      device_name = argv[++i];
      continue;
    }
    if (arg == "--tx_power") {
      if (i + 1 >= argc) {
        std::cerr << "Missing value for --tx_power\n";
        return 2;
      }
      tx_power = std::atoi(argv[++i]);
      continue;
    }
    std::cerr << "Unknown argument: " << arg << "\n";
    PrintUsage(argv[0]);
    return 2;
  }

  struct sigaction action {};
  action.sa_sigaction = QuitHandler;
  action.sa_flags = SA_SIGINFO;
  sigaction(SIGINT, &action, nullptr);
  sigaction(SIGTERM, &action, nullptr);

  nearby::SingleThreadExecutor executor;
  nearby::presence::Mediums mediums;
  DemoCredentialManager credential_manager;
  nearby::presence::BroadcastManager manager(mediums, credential_manager,
                                             executor);

  nearby::presence::PresenceBroadcast::BroadcastSection section = {
      .identity = nearby::internal::IdentityType::IDENTITY_TYPE_PUBLIC,
      .extended_properties = {
          nearby::presence::DataElement(
              nearby::presence::ActionBit::kNearbyShareAction),
        nearby::presence::DataElement(
            nearby::presence::ActionBit::kPresenceManagerAction),
    },
      .account_name = "",
      .manager_app_id = "",
  };
  nearby::presence::PresenceBroadcast presence_broadcast = {
      .sections = {section},
  };
  nearby::presence::BroadcastRequest request = {
      .tx_power = tx_power,
      .power_mode = nearby::presence::PowerMode::kLowLatency,
      .variant = presence_broadcast,
  };

  absl::Status broadcast_status;
  absl::Notification started;
  auto session = manager.StartBroadcast(
      request,
      nearby::presence::BroadcastCallback{
          .start_broadcast_cb =
              [&broadcast_status, &started](absl::Status status) {
                broadcast_status = status;
                started.Notify();
              },
      });
  if (!session.ok()) {
    std::cerr << "Failed to initiate broadcast: " << session.status() << "\n";
    return 1;
  }

  started.WaitForNotification();
  if (!broadcast_status.ok()) {
    std::cerr << "Broadcast start failed: " << broadcast_status << "\n";
    manager.StopBroadcast(*session);
    executor.Shutdown();
    return 1;
  }

  std::cout << "Nearby Presence advertising started.\n"
            << "  device_name: " << device_name << "\n"
            << "  session_id: " << *session << "\n"
            << "Press Ctrl+C to stop.\n";

  g_shutdown.WaitForNotification();
  manager.StopBroadcast(*session);
  executor.Shutdown();
  std::cout << "Stopped Presence advertising.\n";
  return 0;
}
