# Nearby Sharing Linux - Quick Reference

## Quick Start

### Build
```bash
bazel build //sharing/linux:simple_example
bazel build //sharing/linux:nearby_sharing_app
```

### Run Simple Example
```bash
# Terminal 1 (Receiver)
./bazel-bin/sharing/linux/simple_example receiver

# Terminal 2 (Sender)
./bazel-bin/sharing/linux/simple_example sender "Hello World!"
```

### Run Full App
```bash
./bazel-bin/sharing/linux/nearby_sharing_app [device_name]
```

## API Cheat Sheet

### Include Headers
```cpp
#include "sharing/linux/nearby_sharing_service_linux.h"
#include "sharing/attachment_container.h"
#include "sharing/file_attachment.h"
#include "sharing/text_attachment.h"
#include "sharing/share_target.h"
#include "sharing/transfer_metadata.h"
```

### Create Service
```cpp
using namespace nearby::sharing::linux;
NearbySharingServiceLinux service("DeviceName");
```

### Implement Callbacks
```cpp
// Transfer updates
class MyCallback : public TransferUpdateCallback {
  void OnTransferUpdate(const ShareTarget& target,
                       const AttachmentContainer& attachments,
                       const TransferMetadata& metadata) override {
    // Handle status changes
  }
};

// Device discovery
class MyDiscovery : public ShareTargetDiscoveredCallback {
  void OnShareTargetDiscovered(const ShareTarget& target) override { }
  void OnShareTargetLost(const ShareTarget& target) override { }
  void OnShareTargetUpdated(const ShareTarget& target) override { }
};
```

### Register to Receive
```cpp
service.RegisterReceiveSurface(
    &transfer_callback,
    NearbySharingService::ReceiveSurfaceState::kForeground,
    Advertisement::BlockedVendorId::kNone,
    [](auto status) { /* callback */ });
```

### Register to Send
```cpp
service.RegisterSendSurface(
    &transfer_callback,
    &discovery_callback,
    NearbySharingService::SendSurfaceState::kForeground,
    Advertisement::BlockedVendorId::kNone,
    false, // disable_wifi_hotspot
    [](auto status) { /* callback */ });
```

### Send File
```cpp
auto container = std::make_unique<AttachmentContainer>();
container->AddFileAttachment(FileAttachment(FilePath("/path/to/file")));
service.SendAttachments(target_id, std::move(container), [](auto) {});
```

### Send Text
```cpp
auto container = std::make_unique<AttachmentContainer>();
container->AddTextAttachment(TextAttachment(
    TextAttachment::Type::TEXT, "message", std::nullopt, std::nullopt));
service.SendAttachments(target_id, std::move(container), [](auto) {});
```

### Accept/Reject/Cancel
```cpp
service.Accept(target_id, [](auto status) {});
service.Reject(target_id, [](auto status) {});
service.Cancel(target_id, [](auto status) {});
```

### Check Status
```cpp
bool scanning = service.IsScanning();
bool transferring = service.IsTransferring();
bool bt_present = service.IsBluetoothPresent();
bool bt_powered = service.IsBluetoothPowered();
```

### Shutdown
```cpp
service.Shutdown([](auto status) {});
```

## Transfer Statuses

| Status | Meaning | Action |
|--------|---------|--------|
| `kConnecting` | Establishing connection | Wait |
| `kAwaitingLocalConfirmation` | Need to accept/reject | Call Accept() or Reject() |
| `kAwaitingRemoteAcceptance` | Waiting for remote | Wait |
| `kInProgress` | Transferring data | Show progress |
| `kComplete` | Success | Access attachments |
| `kFailed` | Error occurred | Check logs |
| `kRejected` | User rejected | Retry or cancel |
| `kCancelled` | Transfer cancelled | Cleanup |
| `kTimedOut` | Connection timeout | Retry |

## Status Codes

| Code | Meaning |
|------|---------|
| `kOk` | Success |
| `kError` | General error |
| `kOutOfOrderApiCall` | API called incorrectly |
| `kTransferAlreadyInProgress` | Can't start new transfer |
| `kNoAvailableConnectionMedium` | No Bluetooth/WiFi |
| `kInvalidArgument` | Bad parameters |

## Common Patterns

### Auto-Accept Pattern
```cpp
class AutoAccept : public TransferUpdateCallback {
  void OnTransferUpdate(...) override {
    if (metadata.status() == Status::kAwaitingLocalConfirmation) {
      service_->Accept(target.id, [](auto) {});
    }
  }
};
```

### Progress Tracking Pattern
```cpp
void OnTransferUpdate(...) override {
  if (metadata.status() == Status::kInProgress) {
    int percent = metadata.progress() * 100;
    uint64_t bytes = metadata.transferred_bytes();
    std::cout << percent << "% (" << bytes << " bytes)" << std::endl;
  }
}
```

### Device Selection Pattern
```cpp
std::vector<ShareTarget> devices;

void OnShareTargetDiscovered(const ShareTarget& target) override {
  devices.push_back(target);
  std::cout << devices.size() << ". " << target.device_name << std::endl;
}

void SendToDevice(size_t index) {
  if (index < devices.size()) {
    SendFile(devices[index].id, file_path);
  }
}
```

### Error Handling Pattern
```cpp
service.SendAttachments(target_id, container,
    [](NearbySharingService::StatusCodes status) {
      if (status != StatusCodes::kOk) {
        std::cerr << "Error: " 
                 << NearbySharingService::StatusCodeToString(status) 
                 << std::endl;
        return;
      }
      std::cout << "Transfer initiated" << std::endl;
    });
```

## Debugging Tips

### Enable Verbose Logging
```cpp
// Set environment variable
export NEARBY_LOGS=VERBOSE
```

### Check Bluetooth
```cpp
if (!service.IsBluetoothPresent()) {
  std::cout << "No Bluetooth adapter found" << std::endl;
}
if (!service.IsBluetoothPowered()) {
  std::cout << "Bluetooth is off" << std::endl;
}
```

### Dump Service State
```cpp
std::cout << service.Dump() << std::endl;
// Output: "NearbySharingServiceLinux advertising=true scanning=false ..."
```

### Monitor Callbacks
```cpp
void OnTransferUpdate(...) override {
  std::cout << "[Transfer] " << target.device_name 
           << " - " << TransferMetadata::StatusToString(metadata.status())
           << " - " << (metadata.progress() * 100) << "%" << std::endl;
}
```

## File Locations

- **Service**: `sharing/linux/nearby_sharing_service_linux.{h,cc}`
- **Simple Example**: `sharing/linux/simple_example.cc`
- **Full App**: `sharing/linux/nearby_sharing_app.cc`
- **README**: `sharing/linux/README.md`
- **Implementation Guide**: `sharing/linux/IMPLEMENTATION_GUIDE.md`
- **BUILD**: `sharing/linux/BUILD`

## Common Issues

### "No devices found"
- Ensure receiver is running and advertising
- Check Bluetooth is enabled on both devices
- Verify devices are within range (~10m)
- Try restarting Bluetooth

### "Transfer failed"
- Check file permissions
- Verify disk space
- Ensure stable connection
- Check firewall settings

### "Invalid argument"
- Verify target_id is valid
- Ensure container has attachments
- Check surface is registered

### Callback not called
- Verify callback lifetime (must outlive service)
- Check registration was successful
- Ensure main thread/event loop is running

## Example Workflows

### Send File Workflow
```
1. Create service
2. Create callbacks
3. RegisterSendSurface (foreground)
4. Wait for OnShareTargetDiscovered
5. Create AttachmentContainer
6. Add FileAttachment
7. SendAttachments(target_id, container)
8. Wait for kComplete in OnTransferUpdate
```

### Receive File Workflow
```
1. Create service
2. Create callback
3. RegisterReceiveSurface (foreground)
4. Wait for kAwaitingLocalConfirmation
5. Call Accept(target_id)
6. Wait for kInProgress updates
7. Wait for kComplete
8. Access files from AttachmentContainer
```

## Performance Notes

- **Scanning**: Consumes battery, stop when not needed
- **Advertising**: Minimal impact
- **Transfer**: WiFi Direct faster than Bluetooth
- **File Size**: Large files (>100MB) benefit from WiFi
- **Small Files**: Bluetooth sufficient for <10MB

## Links

- [README.md](README.md) - Overview and features
- [IMPLEMENTATION_GUIDE.md](IMPLEMENTATION_GUIDE.md) - Detailed architecture
- [nearby_sharing_service.h](../nearby_sharing_service.h) - Base interface
