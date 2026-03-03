// Copyright 2025 Google LLC
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

#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <chrono>
#include <vector>

#include "absl/time/time.h"
#include "internal/platform/implementation/account_manager.h"
#include "sharing/linux/nearby_sharing_service_linux.h"
#include "sharing/attachment_container.h"
#include "sharing/file_attachment.h"
#include "sharing/share_target.h"
#include "sharing/text_attachment.h"
#include "sharing/transfer_metadata.h"
#include "sharing/share_target_discovered_callback.h"
#include "sharing/transfer_update_callback.h"
#include "internal/base/file_path.h"
#include "internal/base/files.h"

using namespace nearby::sharing;
using namespace nearby::sharing::linux;

class MyTransferUpdateCallback : public TransferUpdateCallback {
 public:
  void OnTransferUpdate(const ShareTarget& share_target,
                       const AttachmentContainer& attachment_container,
                       const TransferMetadata& transfer_metadata) override {
    std::cout << "\n┌─────────────────────────────────────┐" << std::endl;
    std::cout << "│     TRANSFER UPDATE                 │" << std::endl;
    std::cout << "├─────────────────────────────────────┤" << std::endl;
    std::cout << "│ Device: " << share_target.device_name << std::endl;
    std::cout << "│ Target ID: " << share_target.id << " ← USE THIS ID" << std::endl;
    std::cout << "│ Status: " << TransferMetadata::StatusToString(transfer_metadata.status()) << std::endl;
    std::cout << "│ Progress: " << (transfer_metadata.progress() * 100) << "%" << std::endl;
    std::cout << "│ Transferred: " << transfer_metadata.transferred_bytes() << " bytes" << std::endl;
    std::cout << "│ Total attachments: " << transfer_metadata.total_attachments_count() << std::endl;
    
    // Track incoming transfer requests
    if (transfer_metadata.status() == TransferMetadata::Status::kAwaitingLocalConfirmation) {
      std::cout << "├─────────────────────────────────────┤" << std::endl;
      std::cout << "│ ⚠️  INCOMING TRANSFER REQUEST       │" << std::endl;
      std::cout << "│ From: " << share_target.device_name << std::endl;
      std::cout << "│ Target ID: " << share_target.id << std::endl;
      std::cout << "│ → Press 6 to ACCEPT                 │" << std::endl;
      std::cout << "│ → Press 7 to REJECT                 │" << std::endl;
      
      pending_target_id_ = share_target.id;
      has_pending_request_ = true;
    }
    
    // Show completed transfers
    if (transfer_metadata.status() == TransferMetadata::Status::kComplete) {
      std::cout << "├─────────────────────────────────────┤" << std::endl;
      std::cout << "│ ✓ TRANSFER COMPLETE                 │" << std::endl;
      for (const auto& file : attachment_container.GetFileAttachments()) {
        std::cout << "│ Received file: " << file.file_name() << std::endl;
      }
      for (const auto& text : attachment_container.GetTextAttachments()) {
        std::cout << "│ Received text: " << text.text_body() << std::endl;
      }
      has_pending_request_ = false;
    }
    
    std::cout << "└─────────────────────────────────────┘" << std::endl;
    PrintMenuAtBottom();
  }
  
  bool HasPendingRequest() const { return has_pending_request_; }
  int64_t GetPendingTargetId() const { return pending_target_id_; }

 private:
  bool has_pending_request_ = false;
  int64_t pending_target_id_ = -1;
  
  void PrintMenuAtBottom() {
    std::cout << "\n╔═════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║           NEARBY SHARING - MENU                         ║" << std::endl;
    std::cout << "╠═════════════════════════════════════════════════════════╣" << std::endl;
    std::cout << "║ 1. Start as Receiver     │ 6. Accept pending request   ║" << std::endl;
    std::cout << "║ 2. Start as Sender       │ 7. Reject pending request   ║" << std::endl;
    std::cout << "║ 3. List devices          │ 8. Cancel transfer          ║" << std::endl;
    std::cout << "║ 4. Send file             │ 9. Print status             ║" << std::endl;
    std::cout << "║ 5. Send text             │ 0. Exit                     ║" << std::endl;
    std::cout << "║10. Sync credentials      │11. Credential status        ║" << std::endl;
    std::cout << "║12. Visibility: Everyone  │13. Visibility: Contacts     ║" << std::endl;
    std::cout << "║14. Visibility: Hidden    │                             ║" << std::endl;
    std::cout << "╚═════════════════════════════════════════════════════════╝" << std::endl;
    std::cout << "Choice: ";
  }
};

class MyShareTargetDiscoveredCallback : public ShareTargetDiscoveredCallback {
 public:
  void OnShareTargetDiscovered(const ShareTarget& share_target) override {
    std::cout << "\n┌─────────────────────────────────────┐" << std::endl;
    std::cout << "│ 📱 DEVICE DISCOVERED                │" << std::endl;
    std::cout << "├─────────────────────────────────────┤" << std::endl;
    std::cout << "│ ID: " << share_target.id << std::endl;
    std::cout << "│ Name: " << share_target.device_name << std::endl;
    std::cout << "│ Vendor ID: " << static_cast<int>(share_target.vendor_id) << std::endl;
    std::cout << "└─────────────────────────────────────┘" << std::endl;
    
    discovered_targets_.push_back(share_target);
    PrintMenuAtBottom();
  }

  void OnShareTargetLost(const ShareTarget& share_target) override {
    std::cout << "\n┌─────────────────────────────────────┐" << std::endl;
    std::cout << "│ ❌ DEVICE LOST                      │" << std::endl;
    std::cout << "├─────────────────────────────────────┤" << std::endl;
    std::cout << "│ Name: " << share_target.device_name << std::endl;
    std::cout << "└─────────────────────────────────────┘" << std::endl;
    
    for (auto it = discovered_targets_.begin(); it != discovered_targets_.end(); ++it) {
      if (it->id == share_target.id) {
        discovered_targets_.erase(it);
        break;
      }
    }
    PrintMenuAtBottom();
  }

  void OnShareTargetUpdated(const ShareTarget& share_target) override {
    std::cout << "\n┌─────────────────────────────────────┐" << std::endl;
    std::cout << "│ 🔄 DEVICE UPDATED                   │" << std::endl;
    std::cout << "├─────────────────────────────────────┤" << std::endl;
    std::cout << "│ Name: " << share_target.device_name << std::endl;
    std::cout << "└─────────────────────────────────────┘" << std::endl;
    PrintMenuAtBottom();
  }

  const std::vector<ShareTarget>& GetDiscoveredTargets() const {
    return discovered_targets_;
  }

 private:
  std::vector<ShareTarget> discovered_targets_;
  
  void PrintMenuAtBottom() {
    std::cout << "\n╔═════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║           NEARBY SHARING - MENU                         ║" << std::endl;
    std::cout << "╠═════════════════════════════════════════════════════════╣" << std::endl;
    std::cout << "║ 1. Start as Receiver     │ 6. Accept pending request   ║" << std::endl;
    std::cout << "║ 2. Start as Sender       │ 7. Reject pending request   ║" << std::endl;
    std::cout << "║ 3. List devices          │ 8. Cancel transfer          ║" << std::endl;
    std::cout << "║ 4. Send file             │ 9. Print status             ║" << std::endl;
    std::cout << "║ 5. Send text             │ 0. Exit                     ║" << std::endl;
    std::cout << "║10. Sync credentials      │11. Credential status        ║" << std::endl;
    std::cout << "║12. Visibility: Everyone  │13. Visibility: Contacts     ║" << std::endl;
    std::cout << "║14. Visibility: Hidden    │                             ║" << std::endl;
    std::cout << "╚═════════════════════════════════════════════════════════╝" << std::endl;
    std::cout << "Choice: ";
  }
};

class NearbySharingApp {
 public:
  NearbySharingApp(const std::string& device_name) 
      : service_(std::make_unique<NearbySharingServiceLinux>(device_name)),
        transfer_callback_(std::make_unique<MyTransferUpdateCallback>()),
        discovery_callback_(std::make_unique<MyShareTargetDiscoveredCallback>()) {
    std::cout << "Nearby Sharing Application initialized with device name: " 
              << device_name << std::endl;
  }

  ~NearbySharingApp() {
    Shutdown();
  }

  void StartAsReceiver() {
    PrepareCredentialFlow(/*for_receiver=*/true);

    std::cout << "\n=== Starting as Receiver (Foreground) ===" << std::endl;
    
    service_->RegisterReceiveSurface(
        transfer_callback_.get(),
        NearbySharingService::ReceiveSurfaceState::kForeground,
        Advertisement::BlockedVendorId::kNone,
        [this](NearbySharingService::StatusCodes status) {
          if (status == NearbySharingService::StatusCodes::kOk) {
            receive_surface_state_ =
                NearbySharingService::ReceiveSurfaceState::kForeground;
            std::cout << "Successfully registered as receiver!" << std::endl;
            
            // Display QR code URL
            std::string qr_url = service_->GetQrCodeUrl();
            if (!qr_url.empty()) {
              std::cout << "\n┌──────────────────────────────────────────────────────────────┐" << std::endl;
              std::cout << "│ QR CODE URL:                                                 │" << std::endl;
              std::cout << "│ " << qr_url;
              // Add padding if needed
              if (qr_url.length() < 61) {
                std::cout << std::string(61 - qr_url.length(), ' ');
              }
              std::cout << "│" << std::endl;
              std::cout << "│                                                              │" << std::endl;
              std::cout << "│ Scan this with your phone to connect!                        │" << std::endl;
              std::cout << "└──────────────────────────────────────────────────────────────┘" << std::endl;
            }
            ForceCredentialSync("receiver surface registration");
          } else {
            std::cout << "Failed to register as receiver: " 
                     << NearbySharingService::StatusCodeToString(status) << std::endl;
          }
        });
    
    std::cout << "Advertising enabled. Waiting for incoming connections..." << std::endl;
  }

  void StartAsSender() {
    PrepareCredentialFlow(/*for_receiver=*/false);

    std::cout << "\n=== Starting as Sender (Foreground) ===" << std::endl;
    
    service_->RegisterSendSurface(
        transfer_callback_.get(),
        discovery_callback_.get(),
        NearbySharingService::SendSurfaceState::kForeground,
        Advertisement::BlockedVendorId::kNone,
        false, // disable_wifi_hotspot
        [this](NearbySharingService::StatusCodes status) {
          if (status == NearbySharingService::StatusCodes::kOk) {
            send_surface_state_ = NearbySharingService::SendSurfaceState::kForeground;
            std::cout << "Successfully registered as sender!" << std::endl;
            
            // Display QR code URL
            std::string qr_url = service_->GetQrCodeUrl();
            if (!qr_url.empty()) {
              std::cout << "\n┌──────────────────────────────────────────────────────────────┐" << std::endl;
              std::cout << "│ QR CODE URL:                                                 │" << std::endl;
              std::cout << "│ " << qr_url;
              // Add padding if needed
              if (qr_url.length() < 61) {
                std::cout << std::string(61 - qr_url.length(), ' ');
              }
              std::cout << "│" << std::endl;
              std::cout << "│                                                              │" << std::endl;
              std::cout << "│ Scan this with your phone to connect!                        │" << std::endl;
              std::cout << "└──────────────────────────────────────────────────────────────┘" << std::endl;
            }
            ForceCredentialSync("sender surface registration");
          } else {
            std::cout << "Failed to register as sender: " 
                     << NearbySharingService::StatusCodeToString(status) << std::endl;
          }
        });
    
    std::cout << "Scanning for nearby devices..." << std::endl;
  }

  void SendFile(int64_t target_id, const std::string& file_path) {
    std::cout << "\n=== Sending File ===" << std::endl;
    std::cout << "Target ID: " << target_id << std::endl;
    std::cout << "File: " << file_path << std::endl;
    
    nearby::FilePath path(file_path);
    auto file_size = nearby::Files::GetFileSize(path);
    if (!file_size.has_value()) {
      std::cout << "Error: Could not get file size for " << file_path << std::endl;
      return;
    }
    
    // Use the constructor that accepts size
    std::string file_name = path.GetFileName().ToString();
    std::string mime_type = "";  // Will be auto-detected from extension
    
    // Create file attachment with proper size
    FileAttachment file_attachment(
        /*id=*/0,  // ID will be auto-generated
        /*size=*/static_cast<int64_t>(*file_size),
        /*file_name=*/file_name,
        /*mime_type=*/mime_type,
        /*type=*/service::proto::FileMetadata::UNKNOWN,
        /*parent_folder=*/"",
        /*batch_id=*/0
    );
    
    // Set the file path after construction
    file_attachment.set_file_path(path);

    auto attachment_container = AttachmentContainer::Builder()
                                    .AddFileAttachment(
                                        std::move(file_attachment))
                                    .Build();
    
    service_->SendAttachments(
        target_id,
        std::move(attachment_container),
        [](NearbySharingService::StatusCodes status) {
          if (status == NearbySharingService::StatusCodes::kOk) {
            std::cout << "File send initiated successfully!" << std::endl;
          } else {
            std::cout << "Failed to send file: " 
                     << NearbySharingService::StatusCodeToString(status) << std::endl;
          }
        });
  }

  void SendText(int64_t target_id, const std::string& text) {
    std::cout << "\n=== Sending Text ===" << std::endl;
    std::cout << "Target ID: " << target_id << std::endl;
    std::cout << "Text: " << text << std::endl;
    
    auto attachment_container = AttachmentContainer::Builder()
                                    .AddTextAttachment(TextAttachment(
                                        nearby::sharing::service::proto::
                                            TextMetadata::TEXT,
                                        text,
                                        std::nullopt,  // text_title
                                        std::nullopt   // mime_type
                                        ))
                                    .Build();
    
    service_->SendAttachments(
        target_id,
        std::move(attachment_container),
        [](NearbySharingService::StatusCodes status) {
          if (status == NearbySharingService::StatusCodes::kOk) {
            std::cout << "Text send initiated successfully!" << std::endl;
          } else {
            std::cout << "Failed to send text: " 
                     << NearbySharingService::StatusCodeToString(status) << std::endl;
          }
        });
  }

  void AcceptIncomingShare(int64_t target_id) {
    std::cout << "\n=== Accepting Incoming Share ===" << std::endl;
    std::cout << "Target ID: " << target_id << std::endl;
    
    service_->Accept(
        target_id,
        [](NearbySharingService::StatusCodes status) {
          if (status == NearbySharingService::StatusCodes::kOk) {
            std::cout << "Share accepted!" << std::endl;
          } else {
            std::cout << "Failed to accept share: " 
                     << NearbySharingService::StatusCodeToString(status) << std::endl;
          }
        });
  }
  
  void AcceptPendingShare() {
    if (!transfer_callback_->HasPendingRequest()) {
      std::cout << "No pending incoming transfer request!" << std::endl;
      return;
    }
    AcceptIncomingShare(transfer_callback_->GetPendingTargetId());
  }

  void RejectIncomingShare(int64_t target_id) {
    std::cout << "\n=== Rejecting Incoming Share ===" << std::endl;
    std::cout << "Target ID: " << target_id << std::endl;
    
    service_->Reject(
        target_id,
        [](NearbySharingService::StatusCodes status) {
          if (status == NearbySharingService::StatusCodes::kOk) {
            std::cout << "Share rejected!" << std::endl;
          } else {
            std::cout << "Failed to reject share: " 
                     << NearbySharingService::StatusCodeToString(status) << std::endl;
          }
        });
  }
  
  void RejectPendingShare() {
    if (!transfer_callback_->HasPendingRequest()) {
      std::cout << "No pending incoming transfer request!" << std::endl;
      return;
    }
    RejectIncomingShare(transfer_callback_->GetPendingTargetId());
  }

  void CancelTransfer(int64_t target_id) {
    std::cout << "\n=== Canceling Transfer ===" << std::endl;
    std::cout << "Target ID: " << target_id << std::endl;
    
    service_->Cancel(
        target_id,
        [](NearbySharingService::StatusCodes status) {
          if (status == NearbySharingService::StatusCodes::kOk) {
            std::cout << "Transfer cancelled!" << std::endl;
          } else {
            std::cout << "Failed to cancel transfer: " 
                     << NearbySharingService::StatusCodeToString(status) << std::endl;
          }
        });
  }

  void ListDiscoveredDevices() {
    std::cout << "\n=== Discovered Devices ===" << std::endl;
    const auto& targets = discovery_callback_->GetDiscoveredTargets();
    
    if (targets.empty()) {
      std::cout << "No devices found." << std::endl;
    } else {
      for (const auto& target : targets) {
        std::cout << "ID: " << target.id 
                  << " | Name: " << target.device_name 
                  << " | Vendor: " << static_cast<int>(target.vendor_id) << std::endl;
      }
    }
    std::cout << "==========================" << std::endl;
  }

  void PrintStatus() {
    std::cout << "\n=== Service Status ===" << std::endl;
    std::cout << "Bluetooth Present: " << (service_->IsBluetoothPresent() ? "Yes" : "No") << std::endl;
    std::cout << "Bluetooth Powered: " << (service_->IsBluetoothPowered() ? "Yes" : "No") << std::endl;
    std::cout << "Is Scanning: " << (service_->IsScanning() ? "Yes" : "No") << std::endl;
    std::cout << "Is Transferring: " << (service_->IsTransferring() ? "Yes" : "No") << std::endl;
    std::cout << "======================" << std::endl;
  }

  void SyncCredentialsNow() { ForceCredentialSync("manual menu request"); }

  void PrintCredentialStatus() {
    std::cout << "\n=== Credential Flow Status ===" << std::endl;
    auto* account_manager = service_->GetAccountManager();
    if (account_manager == nullptr) {
      std::cout << "Account manager: unavailable on this service implementation."
                << std::endl;
    } else if (account_manager->GetCurrentAccount().has_value()) {
      const auto& account = *account_manager->GetCurrentAccount();
      std::cout << "Signed in account: " << account.email << std::endl;
      std::cout << "Account id: " << account.id << std::endl;
    } else {
      std::cout << "Signed in account: none" << std::endl;
    }

    std::cout << "Certificate manager pointer: "
              << (service_->GetCertificateManager() != nullptr ? "available"
                                                               : "null")
              << std::endl;

    std::cout << "Service dump:\n" << service_->Dump() << std::endl;
    std::cout << "==============================" << std::endl;
  }

  void SetVisibilityEveryone() {
    ApplyVisibility(proto::DeviceVisibility::DEVICE_VISIBILITY_EVERYONE,
                    absl::Minutes(15), "EVERYONE");
  }

  void SetVisibilityContacts() {
    ApplyVisibility(proto::DeviceVisibility::DEVICE_VISIBILITY_ALL_CONTACTS,
                    absl::ZeroDuration(), "ALL_CONTACTS");
  }

  void SetVisibilityHidden() {
    ApplyVisibility(proto::DeviceVisibility::DEVICE_VISIBILITY_HIDDEN,
                    absl::ZeroDuration(), "HIDDEN");
  }

  void Shutdown() {
    std::cout << "\n=== Shutting Down ===" << std::endl;
    service_->Shutdown([](NearbySharingService::StatusCodes status) {
      std::cout << "Shutdown complete: " 
               << NearbySharingService::StatusCodeToString(status) << std::endl;
    });
  }

 private:
  void ApplyVisibility(proto::DeviceVisibility visibility,
                       absl::Duration expiration,
                       const std::string& label) {
    service_->SetVisibility(
        visibility, expiration,
        [label](NearbySharingService::StatusCodes status) mutable {
          if (status == NearbySharingService::StatusCodes::kOk) {
            std::cout << "Visibility updated to " << label << std::endl;
          } else {
            std::cout << "Failed to set visibility to " << label << ": "
                      << NearbySharingService::StatusCodeToString(status)
                      << std::endl;
          }
        });
  }

  void PrepareCredentialFlow(bool for_receiver) {
    auto* account_manager = service_->GetAccountManager();
    bool has_account = account_manager != nullptr &&
                       account_manager->GetCurrentAccount().has_value();

    // Match NearbySharingServiceImpl behavior: outgoing paths are contacts-based
    // when account data is available; receiver mode generally uses everyone.
    if (for_receiver) {
      SetVisibilityEveryone();
    } else if (has_account) {
      SetVisibilityContacts();
    } else {
      SetVisibilityEveryone();
    }
  }

  void ForceCredentialSync(const std::string& reason) {
    std::cout << "[Credential flow] Requesting sync via service hooks ("
              << reason << ")." << std::endl;

    // Keep visibility explicitly valid for cert-backed advertising.
    SetVisibilityEveryone();

    // NearbySharingServiceImpl forces private cert upload from
    // RegisterReceiveSurface when visibility is not hidden. Re-registering
    // the current receive surface is enough to trigger that path.
    auto state = receive_surface_state_.value_or(
        NearbySharingService::ReceiveSurfaceState::kBackground);
    service_->RegisterReceiveSurface(
        transfer_callback_.get(), state, Advertisement::BlockedVendorId::kNone,
        [](NearbySharingService::StatusCodes status) {
          if (status == NearbySharingService::StatusCodes::kOk) {
            std::cout << "[Credential flow] Receive surface refreshed for sync."
                      << std::endl;
          } else {
            std::cout << "[Credential flow] Failed to refresh receive surface: "
                      << NearbySharingService::StatusCodeToString(status)
                      << std::endl;
          }
        });

    // Discovery-triggered public cert download is internal to the full service.
    if (send_surface_state_.has_value()) {
      service_->RegisterSendSurface(
          transfer_callback_.get(), discovery_callback_.get(),
          *send_surface_state_, Advertisement::BlockedVendorId::kNone,
          /*disable_wifi_hotspot=*/false,
          [](NearbySharingService::StatusCodes status) {
            if (status == NearbySharingService::StatusCodes::kOk) {
              std::cout << "[Credential flow] Send surface refreshed for scan "
                           "side sync."
                        << std::endl;
            }
          });
    }
  }

  std::unique_ptr<NearbySharingServiceLinux> service_;
  std::unique_ptr<MyTransferUpdateCallback> transfer_callback_;
  std::unique_ptr<MyShareTargetDiscoveredCallback> discovery_callback_;
  std::optional<NearbySharingService::ReceiveSurfaceState> receive_surface_state_;
  std::optional<NearbySharingService::SendSurfaceState> send_surface_state_;
};

void PrintMenu() {
  std::cout << "\n╔═════════════════════════════════════════════════════════╗" << std::endl;
  std::cout << "║           NEARBY SHARING - MENU                         ║" << std::endl;
  std::cout << "╠═════════════════════════════════════════════════════════╣" << std::endl;
  std::cout << "║ 1. Start as Receiver     │ 6. Accept pending request   ║" << std::endl;
  std::cout << "║ 2. Start as Sender       │ 7. Reject pending request   ║" << std::endl;
  std::cout << "║ 3. List devices          │ 8. Cancel transfer          ║" << std::endl;
  std::cout << "║ 4. Send file             │ 9. Print status             ║" << std::endl;
  std::cout << "║ 5. Send text             │ 0. Exit                     ║" << std::endl;
  std::cout << "║10. Sync credentials      │11. Credential status        ║" << std::endl;
  std::cout << "║12. Visibility: Everyone  │13. Visibility: Contacts     ║" << std::endl;
  std::cout << "║14. Visibility: Hidden    │                             ║" << std::endl;
  std::cout << "╚═════════════════════════════════════════════════════════╝" << std::endl;
  std::cout << "Choice: ";
}

int main(int argc, char* argv[]) {
  std::string device_name = "MyLinuxDevice";
  
  if (argc > 1) {
    device_name = argv[1];
  }

  // Clear screen and show header
  std::cout << "\033[2J\033[1;1H";  // Clear screen
  std::cout << "╔═════════════════════════════════════════════════════════╗" << std::endl;
  std::cout << "║                                                         ║" << std::endl;
  std::cout << "║         NEARBY SHARING - LINUX APPLICATION              ║" << std::endl;
  std::cout << "║         Device: " << device_name << std::string(32 - device_name.length(), ' ') << "║" << std::endl;
  std::cout << "║                                                         ║" << std::endl;
  std::cout << "╚═════════════════════════════════════════════════════════╝" << std::endl;
  std::cout << "\n[LOG AREA - Updates will appear above the menu]\n" << std::endl;

  NearbySharingApp app(device_name);

  bool running = true;
  while (running) {
    PrintMenu();
    
    int choice;
    std::cin >> choice;
    
    switch (choice) {
      case 1:
        app.StartAsReceiver();
        break;
        
      case 2:
        app.StartAsSender();
        std::this_thread::sleep_for(std::chrono::seconds(2));
        app.ListDiscoveredDevices();
        break;
        
      case 3:
        app.ListDiscoveredDevices();
        break;
        
      case 4: {
        int64_t target_id;
        std::string file_path;
        std::cout << "Enter target ID: ";
        std::cin >> target_id;
        std::cout << "Enter file path: ";
        std::cin.ignore();
        std::getline(std::cin, file_path);
        app.SendFile(target_id, file_path);
        break;
      }
      
      case 5: {
        int64_t target_id;
        std::string text;
        std::cout << "Enter target ID: ";
        std::cin >> target_id;
        std::cout << "Enter text to send: ";
        std::cin.ignore();
        std::getline(std::cin, text);
        app.SendText(target_id, text);
        break;
      }
      
      case 6:
        app.AcceptPendingShare();
        break;
      
      case 7:
        app.RejectPendingShare();
        break;
      
      case 8: {
        int64_t target_id;
        std::cout << "Enter target ID to cancel: ";
        std::cin >> target_id;
        app.CancelTransfer(target_id);
        break;
      }
      
      case 9:
        app.PrintStatus();
        break;

      case 10:
        app.SyncCredentialsNow();
        break;

      case 11:
        app.PrintCredentialStatus();
        break;

      case 12:
        app.SetVisibilityEveryone();
        break;

      case 13:
        app.SetVisibilityContacts();
        break;

      case 14:
        app.SetVisibilityHidden();
        break;
        
      case 0:
        running = false;
        break;
        
      default:
        std::cout << "Invalid choice. Please try again." << std::endl;
        break;
    }
  }

  std::cout << "\nGoodbye!" << std::endl;
  return 0;
}
