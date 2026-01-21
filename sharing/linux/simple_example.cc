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

// Simple example: Send a text message to nearby device
//
// Usage:
//   Terminal 1 (Receiver): ./simple_receiver
//   Terminal 2 (Sender):   ./simple_sender "Hello World"

#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <chrono>

#include "sharing/linux/nearby_sharing_service_linux.h"
#include "sharing/attachment_container.h"
#include "sharing/text_attachment.h"
#include "sharing/share_target.h"
#include "sharing/transfer_metadata.h"
#include "sharing/transfer_update_callback.h"
#include "sharing/share_target_discovered_callback.h"

using namespace nearby::sharing;
using namespace nearby::sharing::linux;

// Simple callback that automatically accepts incoming transfers
class SimpleReceiverCallback : public TransferUpdateCallback {
 public:
  explicit SimpleReceiverCallback(NearbySharingServiceLinux* service) 
      : service_(service) {}

  void OnTransferUpdate(const ShareTarget& share_target,
                       const AttachmentContainer& attachment_container,
                       const TransferMetadata& transfer_metadata) override {
    std::cout << "\n[Receiver] Transfer Update from: " << share_target.device_name << std::endl;
    std::cout << "[Receiver] Status: " 
              << TransferMetadata::StatusToString(transfer_metadata.status()) << std::endl;

    // Auto-accept incoming transfers
    if (transfer_metadata.status() == TransferMetadata::Status::kAwaitingLocalConfirmation) {
      std::cout << "[Receiver] Auto-accepting transfer..." << std::endl;
      service_->Accept(share_target.id, [](auto status) {
        std::cout << "[Receiver] Accept status: " 
                 << NearbySharingService::StatusCodeToString(status) << std::endl;
      });
    }

    // Show received content when complete
    if (transfer_metadata.status() == TransferMetadata::Status::kComplete) {
      std::cout << "\n[Receiver] ✓ Transfer Complete!" << std::endl;
      
      // Display received text
      for (const auto& text : attachment_container.GetTextAttachments()) {
        std::cout << "[Receiver] Received text: \"" << text.text_body() << "\"" << std::endl;
      }
      
      // Display received files
      for (const auto& file : attachment_container.GetFileAttachments()) {
        std::cout << "[Receiver] Received file: " << file.file_name() << std::endl;
      }
    }
  }

 private:
  NearbySharingServiceLinux* service_;
};

// Callback for discovering nearby devices
class SimpleSenderCallback : public ShareTargetDiscoveredCallback {
 public:
  void OnShareTargetDiscovered(const ShareTarget& share_target) override {
    std::cout << "\n[Sender] Found device: " << share_target.device_name 
              << " (ID: " << share_target.id << ")" << std::endl;
    discovered_target_ = share_target;
    has_target_ = true;
  }

  void OnShareTargetLost(const ShareTarget& share_target) override {
    std::cout << "[Sender] Lost device: " << share_target.device_name << std::endl;
    if (has_target_ && discovered_target_.id == share_target.id) {
      has_target_ = false;
    }
  }

  void OnShareTargetUpdated(const ShareTarget& share_target) override {}

  bool HasTarget() const { return has_target_; }
  const ShareTarget& GetTarget() const { return discovered_target_; }

 private:
  ShareTarget discovered_target_;
  bool has_target_ = false;
};

// Transfer progress callback for sender
class SimpleSenderTransferCallback : public TransferUpdateCallback {
 public:
  void OnTransferUpdate(const ShareTarget& share_target,
                       const AttachmentContainer& attachment_container,
                       const TransferMetadata& transfer_metadata) override {
    std::cout << "[Sender] Transfer to " << share_target.device_name 
              << ": " << TransferMetadata::StatusToString(transfer_metadata.status()) 
              << " (" << (transfer_metadata.progress() * 100) << "%)" << std::endl;

    if (transfer_metadata.status() == TransferMetadata::Status::kComplete) {
      std::cout << "[Sender] ✓ Transfer Complete!" << std::endl;
      transfer_complete_ = true;
    } else if (transfer_metadata.status() == TransferMetadata::Status::kFailed) {
      std::cout << "[Sender] ✗ Transfer Failed!" << std::endl;
      transfer_complete_ = true;
    }
  }

  bool IsTransferComplete() const { return transfer_complete_; }

 private:
  bool transfer_complete_ = false;
};

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cout << "Usage: " << argv[0] << " <receiver|sender> [message]" << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  " << argv[0] << " receiver              # Start as receiver" << std::endl;
    std::cout << "  " << argv[0] << " sender \"Hello!\"     # Send text message" << std::endl;
    return 1;
  }

  std::string mode = argv[1];

  if (mode == "receiver") {
    // ============= RECEIVER MODE =============
    std::cout << "=== Nearby Sharing Receiver ===" << std::endl;
    
    NearbySharingServiceLinux service("Receiver-Device");
    SimpleReceiverCallback callback(&service);

    // Register as a receiver (advertise to nearby devices)
    service.RegisterReceiveSurface(
        &callback,
        NearbySharingService::ReceiveSurfaceState::kForeground,
        Advertisement::BlockedVendorId::kNone,
        [](auto status) {
          if (status == NearbySharingService::StatusCodes::kOk) {
            std::cout << "[Receiver] ✓ Advertising started" << std::endl;
            std::cout << "[Receiver] Waiting for incoming transfers..." << std::endl;
          } else {
            std::cout << "[Receiver] ✗ Failed to start: " 
                     << NearbySharingService::StatusCodeToString(status) << std::endl;
          }
        });

    // Keep running to receive transfers
    std::cout << "\nPress Ctrl+C to exit..." << std::endl;
    while (true) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }

  } else if (mode == "sender") {
    // ============= SENDER MODE =============
    std::string message = "Hello from Nearby Sharing!";
    if (argc > 2) {
      message = argv[2];
    }

    std::cout << "=== Nearby Sharing Sender ===" << std::endl;
    std::cout << "[Sender] Message to send: \"" << message << "\"" << std::endl;

    NearbySharingServiceLinux service("Sender-Device");
    SimpleSenderCallback discovery_callback;
    SimpleSenderTransferCallback transfer_callback;

    // Register as sender (scan for nearby devices)
    service.RegisterSendSurface(
        &transfer_callback,
        &discovery_callback,
        NearbySharingService::SendSurfaceState::kForeground,
        Advertisement::BlockedVendorId::kNone,
        false,
        [](auto status) {
          if (status == NearbySharingService::StatusCodes::kOk) {
            std::cout << "[Sender] ✓ Scanning started" << std::endl;
          } else {
            std::cout << "[Sender] ✗ Failed to start: " 
                     << NearbySharingService::StatusCodeToString(status) << std::endl;
          }
        });

    // Wait for device discovery
    std::cout << "[Sender] Scanning for nearby devices..." << std::endl;
    int wait_time = 0;
    while (!discovery_callback.HasTarget() && wait_time < 10) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
      wait_time++;
      std::cout << "." << std::flush;
    }
    std::cout << std::endl;

    if (!discovery_callback.HasTarget()) {
      std::cout << "[Sender] ✗ No devices found. Make sure receiver is running!" << std::endl;
      return 1;
    }

    // Create text attachment
    auto attachment_container = std::make_unique<AttachmentContainer>();
    attachment_container->AddTextAttachment(TextAttachment(
        nearby::sharing::service::proto::TextMetadata::TEXT,
        message,
        std::nullopt,
        std::nullopt
    ));

    // Send to discovered device
    const auto& target = discovery_callback.GetTarget();
    std::cout << "\n[Sender] Sending to: " << target.device_name << std::endl;
    
    service.SendAttachments(
        target.id,
        std::move(attachment_container),
        [](auto status) {
          std::cout << "[Sender] Send initiated: " 
                   << NearbySharingService::StatusCodeToString(status) << std::endl;
        });

    // Wait for transfer to complete
    std::cout << "[Sender] Transferring..." << std::endl;
    while (!transfer_callback.IsTransferComplete()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    std::cout << "\n[Sender] Done!" << std::endl;

  } else {
    std::cout << "Invalid mode. Use 'receiver' or 'sender'" << std::endl;
    return 1;
  }

  return 0;
}
