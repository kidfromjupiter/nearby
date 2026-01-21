# Nearby Sharing Service Linux - Architecture & Implementation Guide

## Table of Contents
1. [Architecture Overview](#architecture-overview)
2. [How It Works](#how-it-works)
3. [Implementation Guide](#implementation-guide)
4. [Code Examples](#code-examples)
5. [Best Practices](#best-practices)

## Architecture Overview

### Component Hierarchy

```
NearbySharingServiceLinux
├── Connections Core (nearby connections layer)
│   ├── ServiceControllerRouter
│   └── Medium Management (BLE, WiFi)
├── Observers (UI/App notifications)
├── Send Surfaces (outgoing transfers)
│   ├── Transfer Callbacks
│   └── Discovery Callbacks
├── Receive Surfaces (incoming transfers)
│   └── Transfer Callbacks
└── Active Transfers
    ├── Endpoint Mapping
    ├── Transfer State
    └── Attachment Container
```

### Key Classes

**NearbySharingServiceLinux**: Main service class
- Manages discovery, advertising, and transfers
- Built on top of Nearby Connections Core
- Handles lifecycle of send/receive surfaces

**TransferUpdateCallback**: Interface for transfer notifications
- Called on status changes (connecting, in-progress, complete)
- Provides progress information
- Reports errors and completion

**ShareTargetDiscoveredCallback**: Interface for discovery notifications
- Called when devices are found
- Called when devices are lost
- Called when device info updates

**AttachmentContainer**: Container for files and text
- Manages multiple attachments
- Supports files, text, and WiFi credentials
- Handles attachment lifecycle

## How It Works

### 1. Discovery & Advertising Flow

#### Sender (Discovers devices):
```
RegisterSendSurface (Foreground)
    ↓
StartDiscoveryIfNeeded()
    ↓
core_->StartDiscovery()
    ↓
[BLE Scanning Starts]
    ↓
endpoint_found_cb → ParseAdvertisement()
    ↓
ShareTarget created
    ↓
OnShareTargetDiscovered() callback
```

#### Receiver (Advertises availability):
```
RegisterReceiveSurface (Foreground)
    ↓
StartAdvertisingIfNeeded()
    ↓
BuildAdvertisement()
    ↓
core_->StartAdvertising()
    ↓
[BLE Advertising Starts]
    ↓
[Visible to nearby senders]
```

### 2. Connection Establishment

```
Sender                          Receiver
  |                                |
  | RequestConnection()            |
  |------------------------------->|
  |                                | connection_initiated_cb
  |                                | (auto or manual accept)
  | connection_initiated_cb        |
  |<-------------------------------|
  |                                |
  | AcceptConnection()             | AcceptConnection()
  |------------------------------->|
  |<-------------------------------|
  |                                |
  | connection_accepted_cb         | connection_accepted_cb
  |                                |
  [Connected - Ready for transfer]
```

### 3. File Transfer Flow

```
Sender                          Receiver
  |                                |
  | SendAttachments()              |
  | - Create AttachmentContainer   |
  | - Add FileAttachment          |
  |                                |
  | RequestConnection()            |
  |------------------------------->|
  |                   Status: kAwaitingLocalConfirmation
  |                                |
  |                                | Accept()
  |                                |
  | AcceptConnection()             | AcceptConnection()
  | + PayloadListener              | + PayloadListener
  |                                |
  |           Status: kConnecting  |
  |                                |
  | Send Payloads                  |
  |=============================>  | 
  |   (File data chunks)           |
  |                                |
  |        Status: kInProgress     |
  |   Progress: 0% → 100%          |
  |                                |
  | payload_progress_cb            | payload_progress_cb
  |                                |
  |         Status: kComplete      |
  |                                |
```

### 4. Advertisement Format

The service creates custom BLE advertisements with device information:

```
Byte Layout:
[0]      Header Byte
         - Bits 7-5: Version (3 bits)
         - Bit 4: Visibility (0=visible, 1=hidden)
         - Bits 3-1: Device Type (3 bits)
         - Bit 0: Reserved

[1-2]    Salt (2 random bytes)

[3-16]   Metadata Key (14 bytes - for encryption)

[17+]    TLV Fields (Type-Length-Value)
         - Vendor ID (1 byte)
         - QR Code data (variable)
         - Other metadata

[N+]     Device Name (optional, UTF-8)
```

**Device Types:**
- 0: Unknown
- 1: Phone
- 2: Tablet
- 3: Laptop
- 4: Unknown

### 5. State Management

```cpp
struct TransferState {
  AttachmentContainer attachments;     // Files/text being transferred
  TransferUpdateCallback* callback;    // Where to send updates
  bool is_incoming;                    // Direction of transfer
};

// Mappings
endpoint_to_target_      // endpoint_id → ShareTarget
target_id_to_endpoint_   // share_target_id → endpoint_id
active_transfers_        // endpoint_id → TransferState
```

## Implementation Guide

### Step 1: Create Service Instance

```cpp
#include "sharing/linux/nearby_sharing_service_linux.h"

// Create service with custom device name
NearbySharingServiceLinux service("MyLinuxDevice");

// Or let it auto-detect from system
NearbySharingServiceLinux service;
```

### Step 2: Implement Callbacks

```cpp
class MyTransferCallback : public TransferUpdateCallback {
 public:
  void OnTransferUpdate(const ShareTarget& share_target,
                       const AttachmentContainer& attachment_container,
                       const TransferMetadata& transfer_metadata) override {
    // Handle transfer status changes
    switch (transfer_metadata.status()) {
      case TransferMetadata::Status::kAwaitingLocalConfirmation:
        // Incoming transfer - need to accept/reject
        HandleIncomingRequest(share_target);
        break;
        
      case TransferMetadata::Status::kInProgress:
        // Show progress
        UpdateProgress(transfer_metadata.progress());
        break;
        
      case TransferMetadata::Status::kComplete:
        // Transfer done - access attachments
        HandleCompletedTransfer(attachment_container);
        break;
        
      case TransferMetadata::Status::kFailed:
        // Handle error
        HandleError();
        break;
    }
  }
};

class MyDiscoveryCallback : public ShareTargetDiscoveredCallback {
 public:
  void OnShareTargetDiscovered(const ShareTarget& share_target) override {
    // New device found
    devices_.push_back(share_target);
    NotifyUI();
  }
  
  void OnShareTargetLost(const ShareTarget& share_target) override {
    // Device went away
    RemoveDevice(share_target.id);
  }
  
  void OnShareTargetUpdated(const ShareTarget& share_target) override {
    // Device info changed
    UpdateDevice(share_target);
  }
};
```

### Step 3: Register Surfaces

```cpp
MyTransferCallback transfer_callback;
MyDiscoveryCallback discovery_callback;

// To receive files
service.RegisterReceiveSurface(
    &transfer_callback,
    NearbySharingService::ReceiveSurfaceState::kForeground,
    Advertisement::BlockedVendorId::kNone,
    [](auto status) {
      if (status == NearbySharingService::StatusCodes::kOk) {
        std::cout << "Now advertising to nearby devices" << std::endl;
      }
    });

// To send files
service.RegisterSendSurface(
    &transfer_callback,
    &discovery_callback,
    NearbySharingService::SendSurfaceState::kForeground,
    Advertisement::BlockedVendorId::kNone,
    false, // don't disable wifi hotspot
    [](auto status) {
      if (status == NearbySharingService::StatusCodes::kOk) {
        std::cout << "Now scanning for nearby devices" << std::endl;
      }
    });
```

### Step 4: Send Content

```cpp
// Send a file
void SendFile(int64_t target_id, const std::string& file_path) {
  auto container = std::make_unique<AttachmentContainer>();
  
  FileAttachment attachment(FilePath(file_path));
  container->AddFileAttachment(std::move(attachment));
  
  service.SendAttachments(target_id, std::move(container),
      [](auto status) {
        std::cout << "Send status: " 
                 << NearbySharingService::StatusCodeToString(status) 
                 << std::endl;
      });
}

// Send text
void SendText(int64_t target_id, const std::string& text) {
  auto container = std::make_unique<AttachmentContainer>();
  
  TextAttachment attachment(
      TextAttachment::Type::TEXT,
      text,
      std::nullopt,  // no title
      std::nullopt   // no mime type
  );
  container->AddTextAttachment(std::move(attachment));
  
  service.SendAttachments(target_id, std::move(container),
      [](auto status) { /* ... */ });
}
```

### Step 5: Handle Incoming Transfers

```cpp
void HandleIncomingRequest(const ShareTarget& target) {
  // Show confirmation dialog to user
  std::cout << "Accept file from " << target.device_name << "? (y/n): ";
  char choice;
  std::cin >> choice;
  
  if (choice == 'y') {
    service.Accept(target.id, [](auto status) {
      std::cout << "Accepted!" << std::endl;
    });
  } else {
    service.Reject(target.id, [](auto status) {
      std::cout << "Rejected!" << std::endl;
    });
  }
}

void HandleCompletedTransfer(const AttachmentContainer& container) {
  // Process received files
  for (const auto& file : container.GetFileAttachments()) {
    std::cout << "Received: " << file.file_name() << std::endl;
    if (file.file_path().has_value()) {
      std::cout << "Saved to: " << file.file_path()->string() << std::endl;
    }
  }
  
  // Process received text
  for (const auto& text : container.GetTextAttachments()) {
    std::cout << "Received text: " << text.text_body() << std::endl;
  }
}
```

## Code Examples

### Example 1: Simple File Sender

```cpp
#include "sharing/linux/nearby_sharing_service_linux.h"
#include "sharing/file_attachment.h"
#include <thread>

int main() {
  NearbySharingServiceLinux service("FileSender");
  
  // Setup callbacks
  class SimpleCallback : public TransferUpdateCallback {
    void OnTransferUpdate(...) override {
      std::cout << "Progress: " << transfer_metadata.progress() * 100 << "%" << std::endl;
    }
  } transfer_cb;
  
  class SimpleDiscovery : public ShareTargetDiscoveredCallback {
    int64_t target_id = -1;
    void OnShareTargetDiscovered(const ShareTarget& t) override {
      target_id = t.id;
      std::cout << "Found: " << t.device_name << std::endl;
    }
    void OnShareTargetLost(...) override {}
    void OnShareTargetUpdated(...) override {}
  } discovery_cb;
  
  // Start scanning
  service.RegisterSendSurface(&transfer_cb, &discovery_cb,
      NearbySharingService::SendSurfaceState::kForeground,
      Advertisement::BlockedVendorId::kNone, false, [](auto) {});
  
  // Wait for discovery
  std::this_thread::sleep_for(std::chrono::seconds(5));
  
  if (discovery_cb.target_id != -1) {
    // Send file
    auto container = std::make_unique<AttachmentContainer>();
    container->AddFileAttachment(FileAttachment(FilePath("/path/to/file.txt")));
    service.SendAttachments(discovery_cb.target_id, std::move(container), [](auto) {});
    
    // Wait for completion
    std::this_thread::sleep_for(std::chrono::seconds(10));
  }
  
  return 0;
}
```

### Example 2: Auto-Accepting Receiver

```cpp
class AutoAcceptCallback : public TransferUpdateCallback {
 public:
  AutoAcceptCallback(NearbySharingServiceLinux* service) : service_(service) {}
  
  void OnTransferUpdate(const ShareTarget& share_target,
                       const AttachmentContainer& attachment_container,
                       const TransferMetadata& transfer_metadata) override {
    // Auto-accept all incoming transfers
    if (transfer_metadata.status() == TransferMetadata::Status::kAwaitingLocalConfirmation) {
      service_->Accept(share_target.id, [](auto) {});
    }
    
    // Save received files
    if (transfer_metadata.status() == TransferMetadata::Status::kComplete) {
      for (const auto& file : attachment_container.GetFileAttachments()) {
        std::cout << "Saved: " << file.file_name() << std::endl;
      }
    }
  }
  
 private:
  NearbySharingServiceLinux* service_;
};

int main() {
  NearbySharingServiceLinux service("AutoReceiver");
  AutoAcceptCallback callback(&service);
  
  service.RegisterReceiveSurface(&callback,
      NearbySharingService::ReceiveSurfaceState::kForeground,
      Advertisement::BlockedVendorId::kNone, [](auto) {});
  
  // Keep running
  while (true) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
}
```

## Best Practices

### 1. Callback Lifetime Management

```cpp
// DON'T: Callbacks going out of scope
void BadExample() {
  MyTransferCallback callback;  // Stack allocated
  service.RegisterSendSurface(&callback, ...);
  // callback destroyed when function exits!
}

// DO: Keep callbacks alive
class App {
  MyTransferCallback callback_;  // Member variable
  
  void Setup() {
    service.RegisterSendSurface(&callback_, ...);
  }
};
```

### 2. Error Handling

```cpp
service.SendAttachments(target_id, container, 
    [this](NearbySharingService::StatusCodes status) {
      switch (status) {
        case StatusCodes::kOk:
          // Success
          break;
        case StatusCodes::kInvalidArgument:
          // Bad target_id or empty container
          LogError("Invalid arguments");
          break;
        case StatusCodes::kNoAvailableConnectionMedium:
          // Bluetooth/WiFi not available
          NotifyUserToEnableBluetooth();
          break;
        default:
          LogError("Transfer failed");
          break;
      }
    });
```

### 3. Resource Cleanup

```cpp
class ProperCleanup {
 public:
  ~ProperCleanup() {
    // Unregister surfaces before destroying callbacks
    service_.UnregisterSendSurface(&transfer_callback_, [](auto) {});
    service_.UnregisterReceiveSurface(&transfer_callback_, [](auto) {});
    
    // Shutdown service
    service_.Shutdown([](auto) {});
  }
  
 private:
  NearbySharingServiceLinux service_;
  MyTransferCallback transfer_callback_;
};
```

### 4. Thread Safety

```cpp
// The service is NOT thread-safe
// All calls should be from the same thread or synchronized

class ThreadSafeApp {
 public:
  void SendFromAnyThread(int64_t target_id, const std::string& file) {
    task_runner_.PostTask([this, target_id, file]() {
      // All service calls happen on same thread
      auto container = std::make_unique<AttachmentContainer>();
      container->AddFileAttachment(FileAttachment(FilePath(file)));
      service_.SendAttachments(target_id, std::move(container), [](auto) {});
    });
  }
  
 private:
  NearbySharingServiceLinux service_;
  TaskRunner task_runner_;  // Your threading implementation
};
```

### 5. State Tracking

```cpp
class StatefulApp {
 public:
  void OnTransferUpdate(...) override {
    current_state_ = transfer_metadata.status();
    
    // Track progress
    if (transfer_metadata.status() == Status::kInProgress) {
      progress_map_[share_target.id] = transfer_metadata.progress();
    }
    
    // Cleanup on completion
    if (TransferMetadata::IsFinalStatus(transfer_metadata.status())) {
      progress_map_.erase(share_target.id);
    }
  }
  
 private:
  TransferMetadata::Status current_state_;
  std::unordered_map<int64_t, float> progress_map_;
};
```

## Troubleshooting

### Discovery Not Working
- Check Bluetooth is enabled: `IsBluetoothPowered()`
- Verify sender is in foreground state
- Ensure receiver is advertising
- Check for permission issues

### Transfers Failing
- Verify file paths are valid and accessible
- Check available disk space on receiver
- Ensure stable Bluetooth connection
- Monitor transfer callbacks for specific error status

### Connection Issues
- Devices must be within Bluetooth range (~10m)
- Minimize interference from other BLE devices
- Ensure both devices support required BLE features
- Check firewall settings for WiFi Direct

## Performance Tips

1. **Use appropriate surface states**: Background mode when not actively transferring
2. **Unregister when not needed**: Stop scanning/advertising to save battery
3. **Batch small files**: Combine into zip for better efficiency
4. **Monitor transfer progress**: Cancel stalled transfers
5. **Handle errors gracefully**: Retry with exponential backoff

