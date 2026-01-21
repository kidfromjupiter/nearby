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

#include "sharing/linux/nearby_sharing_service_linux.h"
#include "sharing/attachment_container.h"
#include "sharing/file_attachment.h"
#include "sharing/text_attachment.h"
#include "sharing/share_target.h"
#include "sharing/transfer_metadata.h"
#include "sharing/transfer_update_callback.h"
#include "sharing/share_target_discovered_callback.h"
#include "internal/base/file_path.h"

using namespace nearby::sharing;
using namespace nearby::sharing::linux;

class MyTransferUpdateCallback : public TransferUpdateCallback {
 public:
  void OnTransferUpdate(const ShareTarget& share_target,
                       const AttachmentContainer& attachment_container,
                       const TransferMetadata& transfer_metadata) override {
    std::cout << "\n=== Transfer Update ===" << std::endl;
    std::cout << "Device: " << share_target.device_name << std::endl;
    std::cout << "Status: " << TransferMetadata::StatusToString(transfer_metadata.status()) << std::endl;
    std::cout << "Progress: " << (transfer_metadata.progress() * 100) << "%" << std::endl;
    std::cout << "Transferred: " << transfer_metadata.transferred_bytes() << " bytes" << std::endl;
    std::cout << "Total attachments: " << transfer_metadata.total_attachments_count() << std::endl;
    std::cout << "======================" << std::endl;
  }
};

class MyShareTargetDiscoveredCallback : public ShareTargetDiscoveredCallback {
 public:
  void OnShareTargetDiscovered(const ShareTarget& share_target) override {
    std::cout << "\n*** Device Discovered ***" << std::endl;
    std::cout << "ID: " << share_target.id << std::endl;
    std::cout << "Name: " << share_target.device_name << std::endl;
    std::cout << "Vendor ID: " << static_cast<int>(share_target.vendor_id) << std::endl;
    std::cout << "*************************" << std::endl;
    
    discovered_targets_.push_back(share_target);
  }

  void OnShareTargetLost(const ShareTarget& share_target) override {
    std::cout << "\n*** Device Lost ***" << std::endl;
    std::cout << "Name: " << share_target.device_name << std::endl;
    std::cout << "*******************" << std::endl;
    
    for (auto it = discovered_targets_.begin(); it != discovered_targets_.end(); ++it) {
      if (it->id == share_target.id) {
        discovered_targets_.erase(it);
        break;
      }
    }
  }

  void OnShareTargetUpdated(const ShareTarget& share_target) override {
    std::cout << "\n*** Device Updated ***" << std::endl;
    std::cout << "Name: " << share_target.device_name << std::endl;
    std::cout << "**********************" << std::endl;
  }

  const std::vector<ShareTarget>& GetDiscoveredTargets() const {
    return discovered_targets_;
  }

 private:
  std::vector<ShareTarget> discovered_targets_;
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
    std::cout << "\n=== Starting as Receiver (Foreground) ===" << std::endl;
    
    service_->RegisterReceiveSurface(
        transfer_callback_.get(),
        NearbySharingService::ReceiveSurfaceState::kForeground,
        Advertisement::BlockedVendorId::kNone,
        [](NearbySharingService::StatusCodes status) {
          if (status == NearbySharingService::StatusCodes::kOk) {
            std::cout << "Successfully registered as receiver!" << std::endl;
          } else {
            std::cout << "Failed to register as receiver: " 
                     << NearbySharingService::StatusCodeToString(status) << std::endl;
          }
        });
    
    std::cout << "Advertising enabled. Waiting for incoming connections..." << std::endl;
  }

  void StartAsSender() {
    std::cout << "\n=== Starting as Sender (Foreground) ===" << std::endl;
    
    service_->RegisterSendSurface(
        transfer_callback_.get(),
        discovery_callback_.get(),
        NearbySharingService::SendSurfaceState::kForeground,
        Advertisement::BlockedVendorId::kNone,
        false, // disable_wifi_hotspot
        [](NearbySharingService::StatusCodes status) {
          if (status == NearbySharingService::StatusCodes::kOk) {
            std::cout << "Successfully registered as sender!" << std::endl;
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
    
    auto attachment_container = std::make_unique<AttachmentContainer>();
    
    attachment_container->AddFileAttachment(FileAttachment(nearby::FilePath(file_path)));
    
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
    
    auto attachment_container = std::make_unique<AttachmentContainer>();
    
    attachment_container->AddTextAttachment(TextAttachment(
        nearby::sharing::service::proto::TextMetadata::TEXT,
        text,
        std::nullopt, // text_title
        std::nullopt  // mime_type
    ));
    
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

  void Shutdown() {
    std::cout << "\n=== Shutting Down ===" << std::endl;
    service_->Shutdown([](NearbySharingService::StatusCodes status) {
      std::cout << "Shutdown complete: " 
               << NearbySharingService::StatusCodeToString(status) << std::endl;
    });
  }

 private:
  std::unique_ptr<NearbySharingServiceLinux> service_;
  std::unique_ptr<MyTransferUpdateCallback> transfer_callback_;
  std::unique_ptr<MyShareTargetDiscoveredCallback> discovery_callback_;
};

void PrintMenu() {
  std::cout << "\n========== Nearby Sharing Menu ==========" << std::endl;
  std::cout << "1. Start as Receiver (advertise)" << std::endl;
  std::cout << "2. Start as Sender (discover)" << std::endl;
  std::cout << "3. List discovered devices" << std::endl;
  std::cout << "4. Send file to device" << std::endl;
  std::cout << "5. Send text to device" << std::endl;
  std::cout << "6. Accept incoming share" << std::endl;
  std::cout << "7. Reject incoming share" << std::endl;
  std::cout << "8. Cancel transfer" << std::endl;
  std::cout << "9. Print status" << std::endl;
  std::cout << "0. Exit" << std::endl;
  std::cout << "=========================================" << std::endl;
  std::cout << "Enter choice: ";
}

int main(int argc, char* argv[]) {
  std::string device_name = "MyLinuxDevice";
  
  if (argc > 1) {
    device_name = argv[1];
  }

  std::cout << "========================================" << std::endl;
  std::cout << "  Nearby Sharing Linux Application" << std::endl;
  std::cout << "========================================" << std::endl;

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
      
      case 6: {
        int64_t target_id;
        std::cout << "Enter target ID to accept: ";
        std::cin >> target_id;
        app.AcceptIncomingShare(target_id);
        break;
      }
      
      case 7: {
        int64_t target_id;
        std::cout << "Enter target ID to reject: ";
        std::cin >> target_id;
        app.RejectIncomingShare(target_id);
        break;
      }
      
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
