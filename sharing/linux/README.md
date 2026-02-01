# Nearby Sharing Linux Implementation

This directory contains the Linux-specific implementation of Nearby Sharing and a sample application demonstrating its usage.

## ⚠️ Important Compatibility Notice

**This is a simplified implementation for Linux-to-Linux transfers only.**

- ✅ **Works**: Linux ↔ Linux device transfers
- ❌ **Does NOT work**: Linux ↔ Android/ChromeOS (authentication failure)

**Reason**: The full Nearby Sharing protocol requires certificate-based authentication, introduction frame exchange, and protocol frame handling which are not implemented in this simplified version.

**For Android/ChromeOS compatibility**, you need the full implementation in `sharing/nearby_sharing_service_impl.{h,cc}` which requires additional platform support. See [ANDROID_COMPATIBILITY.md](ANDROID_COMPATIBILITY.md) for details.

## Overview

Nearby Sharing is a feature that allows users to share files, text, and other content between nearby devices using Bluetooth Low Energy (BLE) and Wi-Fi Direct. This implementation provides a simplified Linux interface built on top of the Nearby Connections API.

## Components

### NearbySharingServiceLinux

The main service class that provides nearby sharing functionality:

- **Discovery & Advertising**: Find nearby devices and advertise your device's availability
- **File Transfer**: Send and receive files
- **Text Transfer**: Send and receive text messages
- **Connection Management**: Handle connection lifecycle (accept, reject, cancel)

### Key Concepts

#### 1. Send Surface
Represents the sending side of a transfer:
- **Foreground**: Actively scans for nearby devices
- **Background**: Only listens for transfer updates without scanning

#### 2. Receive Surface
Represents the receiving side of a transfer:
- **Foreground**: Advertises to everyone, visible to all nearby devices
- **Background**: Advertises only to contacts (limited visibility)

#### 3. Callbacks

**TransferUpdateCallback**: Receives updates about ongoing transfers
- Status changes (connecting, in progress, complete, failed)
- Progress updates
- Transfer metadata (speed, bytes transferred, etc.)

**ShareTargetDiscoveredCallback**: Receives notifications about discovered devices
- Device discovered
- Device lost (out of range)
- Device updated

#### 4. Attachments

**FileAttachment**: Represents a file to be transferred
- Requires a file path
- Automatically determines MIME type and size

**TextAttachment**: Represents text content to be transferred
- Supports plain text, URLs, addresses, and phone numbers
- Includes optional title and MIME type

## Sample Application

The `nearby_sharing_app.cc` demonstrates how to use the service:

### Building

```bash
# Build the sample application
bazel build //sharing/linux:nearby_sharing_app
```

### Running

```bash
# Run with default device name
./bazel-bin/sharing/linux/nearby_sharing_app

# Run with custom device name
./bazel-bin/sharing/linux/nearby_sharing_app "MyCustomName"
```

### Features

1. **Start as Receiver**: Advertise your device to receive files
2. **Start as Sender**: Discover nearby devices to send files
3. **List Discovered Devices**: View all devices found during scanning
4. **Send File**: Transfer a file to a discovered device
5. **Send Text**: Send text content to a discovered device
6. **Accept/Reject**: Handle incoming transfer requests
7. **Cancel Transfer**: Cancel an ongoing transfer
8. **Status Info**: View Bluetooth and service status

## Usage Examples

### Example 1: Send a File

**Device A (Sender)**:
```cpp
NearbySharingApp app("Sender-Device");

// Start scanning for devices
app.StartAsSender();

// Wait for discovery...
std::this_thread::sleep_for(std::chrono::seconds(3));

// List discovered devices
app.ListDiscoveredDevices();

// Send file to target with ID 1
app.SendFile(1, "/path/to/file.txt");
```

**Device B (Receiver)**:
```cpp
NearbySharingApp app("Receiver-Device");

// Start advertising
app.StartAsReceiver();

// When connection is initiated (via callback), accept it
// This happens automatically when you see OnTransferUpdate with
// Status::kAwaitingLocalConfirmation
app.AcceptIncomingShare(target_id);
```

### Example 2: Send Text

```cpp
NearbySharingApp app("Text-Sender");

// Start as sender
app.StartAsSender();

// Wait for device discovery
std::this_thread::sleep_for(std::chrono::seconds(2));

// Send text to discovered device
app.SendText(1, "Hello from Nearby Sharing!");
```

### Example 3: Custom Callbacks

```cpp
class CustomTransferCallback : public TransferUpdateCallback {
 public:
  void OnTransferUpdate(const ShareTarget& share_target,
                       const AttachmentContainer& attachment_container,
                       const TransferMetadata& transfer_metadata) override {
    switch (transfer_metadata.status()) {
      case TransferMetadata::Status::kAwaitingLocalConfirmation:
        // Auto-accept incoming transfers
        service_->Accept(share_target.id, [](auto status) {
          std::cout << "Auto-accepted" << std::endl;
        });
        break;
        
      case TransferMetadata::Status::kComplete:
        std::cout << "Transfer completed!" << std::endl;
        // Handle completed files from attachment_container
        break;
        
      case TransferMetadata::Status::kFailed:
        std::cout << "Transfer failed!" << std::endl;
        break;
        
      default:
        break;
    }
  }
};
```

## Architecture

### Service Initialization

```cpp
NearbySharingServiceLinux service("DeviceName");
```

The service initializes:
1. **Device Info**: Gets OS device name and type
2. **Bluetooth Adapter**: Checks BT availability
3. **Connections Core**: Sets up the Nearby Connections layer
4. **Service Controller Router**: Manages connection routing

### Discovery Flow

1. **Register Send Surface** (Foreground)
2. Service starts **scanning** for nearby devices
3. When device found: **OnShareTargetDiscovered** callback
4. Advertisement is **parsed** to extract device info
5. **ShareTarget** created with device details
6. User can **select target** and initiate transfer

### Advertising Flow

1. **Register Receive Surface** (Foreground/Background)
2. Service **builds advertisement** with device info
3. Service starts **advertising** via Bluetooth/Wi-Fi
4. When connection requested: **OnTransferUpdate** callback
5. User can **accept/reject** the incoming transfer

### Transfer Flow

#### Sending:
1. **SendAttachments()** with target ID and attachments
2. Service creates **connection request**
3. **Connection initiated** → Status: kConnecting
4. Connection **accepted** → Sends file/text payloads
5. **Payload transfer** → Status: kInProgress
6. **Transfer complete** → Status: kComplete

#### Receiving:
1. **Incoming connection** → Status: kAwaitingLocalConfirmation
2. **Accept()** called → Connection accepted
3. **Receive payloads** → Status: kInProgress
4. **Payloads saved** to local storage
5. **Transfer complete** → Status: kComplete

## Implementation Details

### Advertisement Format

The service uses a custom advertisement format:
- **Header byte**: Version, visibility, device type
- **Salt**: 2 random bytes
- **Metadata key**: 14 bytes (for encryption)
- **TLV fields**: Vendor ID, QR code, etc.
- **Device name** (optional): UTF-8 device name

### Connection Strategy

Uses `P2P_POINT_TO_POINT` strategy:
- Direct peer-to-peer connections
- Supports Bluetooth and Wi-Fi Direct
- Automatic medium selection based on availability

### Medium Selection

The service attempts to use available media:
1. **Bluetooth LE**: For discovery and initial connection
2. **Wi-Fi Direct**: For high-speed file transfer
3. **Wi-Fi LAN**: If devices on same network

### Payload Types

1. **BYTES**: For text content and metadata
2. **FILE**: For file transfers
3. **STREAM**: For real-time data

## API Reference

### Core Methods

#### RegisterSendSurface
```cpp
void RegisterSendSurface(
    TransferUpdateCallback* transfer_callback,
    ShareTargetDiscoveredCallback* discovery_callback,
    SendSurfaceState state,
    Advertisement::BlockedVendorId blocked_vendor_id,
    bool disable_wifi_hotspot,
    std::function<void(StatusCodes)> status_codes_callback);
```

#### RegisterReceiveSurface
```cpp
void RegisterReceiveSurface(
    TransferUpdateCallback* transfer_callback,
    ReceiveSurfaceState state,
    Advertisement::BlockedVendorId vendor_id,
    std::function<void(StatusCodes)> status_codes_callback);
```

#### SendAttachments
```cpp
void SendAttachments(
    int64_t share_target_id,
    std::unique_ptr<AttachmentContainer> attachment_container,
    std::function<void(StatusCodes)> status_codes_callback);
```

#### Accept/Reject/Cancel
```cpp
void Accept(int64_t share_target_id,
           std::function<void(StatusCodes)> status_codes_callback);

void Reject(int64_t share_target_id,
           std::function<void(StatusCodes)> status_codes_callback);

void Cancel(int64_t share_target_id,
           std::function<void(StatusCodes)> status_codes_callback);
```

### Status Codes

- **kOk**: Operation successful
- **kError**: General error
- **kOutOfOrderApiCall**: API called in wrong order
- **kTransferAlreadyInProgress**: Transfer already active
- **kNoAvailableConnectionMedium**: No BT/Wi-Fi available
- **kInvalidArgument**: Invalid parameter provided

## Limitations

Current implementation limitations:
- No settings persistence
- No contact management
- No certificate management
- No account integration
- Limited visibility control
- No Wi-Fi LAN detection
- No extended advertising support

## Future Enhancements

Potential improvements:
1. Add settings persistence (device name, visibility)
2. Implement contact management
3. Add certificate-based authentication
4. Support visibility time limits
5. Add Wi-Fi LAN connectivity detection
6. Implement file path updates during transfer
7. Add QR code generation for pairing
8. Support for extended advertising

## Troubleshooting

### Bluetooth Issues
```cpp
if (!service.IsBluetoothPresent()) {
    std::cout << "Bluetooth adapter not found" << std::endl;
}
if (!service.IsBluetoothPowered()) {
    std::cout << "Bluetooth is disabled" << std::endl;
}
```

### Discovery Not Working
- Ensure Bluetooth is enabled
- Check that sender is in foreground mode
- Verify receiver is advertising
- Check for Bluetooth permissions

### Transfer Failures
- Verify file paths are accessible
- Check available disk space
- Ensure stable Bluetooth connection
- Monitor transfer callbacks for errors

## License

Copyright 2025 Google LLC. Licensed under Apache 2.0.
