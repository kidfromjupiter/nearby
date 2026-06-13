# Nearby Sharing Linux - Known Limitations and Compatibility

## Authentication Failure Issue

### Problem

When trying to connect between the Linux implementation and Android Nearby Share, you may see:
```
[NS_AUTH] send-side ConnectionFailure
AUTH_FAILURE
```

### Root Cause

The current Linux implementation (`nearby_sharing_service_linux`) is a **simplified version** that:
- ✅ Implements device discovery (BLE advertising/scanning)
- ✅ Implements connection establishment (via Nearby Connections)
- ✅ Implements basic payload transfer
- ❌ **Does NOT implement the full Nearby Sharing protocol**

The full Nearby Sharing protocol (used by Android/ChromeOS) requires:

1. **Introduction Frame Exchange**
   - Sender sends an introduction with file metadata
   - Receiver parses and displays file info before accepting

2. **Paired Key Verification** 
   - Security handshake using certificates
   - PIN/Token verification for untrusted devices
   - Requires `NearbyShareCertificateManager`

3. **Protocol Frames**
   - Structured message format (protobuf-based)
   - Control frames for connection, progress, cancellation
   - Metadata exchange before file transfer

4. **Certificate Management**
   - Public/Private certificate pairs
   - Contact-based sharing
   - Visibility controls

### What the Linux Implementation Lacks

```cpp
// Missing in nearby_sharing_service_linux.cc:

// 1. Certificate Manager (returns nullptr)
NearbyShareCertificateManager* GetCertificateManager() {
  return nullptr;  // ❌ Not implemented
}

// 2. Contact Manager (returns nullptr)
NearbyShareContactManager* GetContactManager() {
  return nullptr;  // ❌ Not implemented
}

// 3. Settings Manager (returns nullptr)
NearbyShareSettings* GetSettings() {
  return nullptr;  // ❌ Not implemented
}

// 4. No introduction frame exchange
// 5. No paired key verification
// 6. No protocol frame parsing
```

## Workarounds

### Option 1: Linux-to-Linux Only

The current implementation **DOES work** for Linux-to-Linux transfers:

```bash
# Device 1
./nearby_sharing_app
> Choose 1 (Start as Receiver)

# Device 2  
./nearby_sharing_app
> Choose 2 (Start as Sender)
> Choose 4 (Send file)
```

Both devices use the same simplified protocol, so they can communicate.

### Option 2: Use the Full Implementation

To communicate with Android/ChromeOS, you need the **full Nearby Sharing implementation** which is located in:

```
sharing/nearby_sharing_service_impl.h
sharing/nearby_sharing_service_impl.cc
```

This is the complete implementation used by ChromeOS and includes all the protocol handling.

**However**, this requires:
- Platform-specific implementations (preferences, UI, etc.)
- Certificate storage and management
- Contact synchronization
- More complex setup

### Option 3: Implement Protocol Handlers

Add the missing protocol components to `nearby_sharing_service_linux`:

#### A. Add Introduction Frame Support

```cpp
// sharing/linux/nearby_sharing_service_linux.cc

#include "sharing/proto/wire_format.pb.h"

void NearbySharingServiceLinux::SendIntroductionFrame(
    const std::string& endpoint_id,
    const AttachmentContainer& attachments) {
  
  // Build introduction message
  nearby::sharing::service::proto::V1Frame frame;
  frame.set_type(nearby::sharing::service::proto::V1Frame::INTRODUCTION);
  
  auto* intro = frame.mutable_introduction();
  
  // Add file metadata
  for (const auto& file : attachments.GetFileAttachments()) {
    auto* file_meta = intro->add_file_metadata();
    file_meta->set_name(std::string(file.file_name()));
    file_meta->set_size(file.size());
    file_meta->set_mime_type(std::string(file.mime_type()));
    file_meta->set_type(file.type());
  }
  
  // Add text metadata
  for (const auto& text : attachments.GetTextAttachments()) {
    auto* text_meta = intro->add_text_metadata();
    text_meta->set_text_title(std::string(text.text_title()));
    text_meta->set_size(text.size());
    text_meta->set_type(text.type());
  }
  
  // Serialize and send
  std::string serialized;
  frame.SerializeToString(&serialized);
  
  auto payload = std::make_unique<connections::Payload>(
      ByteArray(serialized));
  
  std::vector<std::string> endpoints = {endpoint_id};
  core_->SendPayload(endpoints, std::move(*payload), [](auto status) {});
}
```

#### B. Add Certificate Stubs

```cpp
// For basic interop without real security:

class SimpleCertificateManager : public NearbyShareCertificateManager {
 public:
  // Return a dummy certificate
  std::optional<NearbySharePrivateCertificate> GetPrivateCertificate() {
    // Create minimal valid certificate
    return CreateDummyCertificate();
  }
};
```

#### C. Handle Paired Key Verification

```cpp
void NearbySharingServiceLinux::HandleConnectionAccepted(...) {
  // Send connection response frame
  nearby::sharing::service::proto::V1Frame frame;
  frame.set_type(V1Frame::CONNECTION_RESPONSE);
  
  auto* response = frame.mutable_connection_response();
  response->set_status(ConnectionResponseFrame::ACCEPT);
  
  // Send frame...
  
  // Then send introduction
  SendIntroductionFrame(endpoint_id, attachments);
  
  // Wait for response before sending payloads
}
```

## Full Protocol Flow (What's Missing)

### Android/ChromeOS Protocol:

```
Sender                          Receiver
  |                                |
  | 1. Request Connection          |
  |------------------------------ >|
  |                                |
  | 2. Connection Initiated        |
  |< -----------------------------|
  |                                |
  | 3. Accept Connection           |
  |------------------------------ >|
  |                                |
  | 4. Send Introduction Frame     | ← MISSING IN LINUX
  |    (file metadata, etc)        |
  |------------------------------ >|
  |                                |
  |                                | 5. Parse Introduction
  |                                | 6. Show UI to user
  |                                | 7. User accepts
  |                                |
  | 8. Send Connection Response    | ← MISSING IN LINUX
  |    (ACCEPT/REJECT)             |
  |< -----------------------------|
  |                                |
  | 9. Paired Key Frame (optional) | ← MISSING IN LINUX
  |< -----------------------------|
  |                                |
  | 10. Send Payloads              |
  |============================== >|
  |                                |
```

### Linux Implementation (Current):

```
Sender                          Receiver
  |                                |
  | 1. Request Connection          |
  |------------------------------ >|
  |                                |
  | 2. Accept Connection           |
  |< -----------------------------|
  |                                |
  | 3. Send Payloads (DIRECTLY)    | ← Android rejects this
  |============================== >|
  |                                |
```

## Recommended Solution

For **production use** with Android/ChromeOS devices:

1. **Use nearby_sharing_service_impl.h** - The full implementation
2. **Implement platform layer** - Provide Linux implementations of:
   - PreferenceManager
   - Context  
   - UI callbacks
   - Certificate storage

Example structure:
```cpp
// Create Linux platform implementation
class LinuxNearbyShareContext : public Context { ... };
class LinuxPreferenceManager : public PreferenceManager { ... };

// Use full service implementation
auto service = NearbySharingServiceFactory::CreateNearbySharingService(
    preference_manager,
    notification_delegate,
    context,
    account_manager,
    device_info);
```

For **testing/development** between Linux devices:

Continue using `nearby_sharing_service_linux` - it works fine for Linux-to-Linux transfers since both sides use the same simplified protocol.

## References

- Full implementation: `sharing/nearby_sharing_service_impl.{h,cc}`
- Protocol definitions: `sharing/proto/wire_format.proto`
- Certificate manager: `sharing/certificates/nearby_share_certificate_manager.h`
- Incoming frames reader: `sharing/incoming_frames_reader.{h,cc}`
- Outgoing frames writer: (implicitly in share sessions)

## Summary

| Feature | Linux Implementation | Full Implementation |
|---------|---------------------|---------------------|
| Discovery | ✅ Working | ✅ Working |
| Advertising | ✅ Working | ✅ Working |
| Connection | ✅ Basic | ✅ Full protocol |
| Introduction frames | ❌ Missing | ✅ Implemented |
| Certificates | ❌ Stub only | ✅ Full PKI |
| Android compat | ❌ Auth fails | ✅ Full compat |
| Linux-to-Linux | ✅ Working | ✅ Working |
| File transfer | ✅ Basic | ✅ Full |
| Progress tracking | ✅ Basic | ✅ Detailed |

The auth failure you're seeing is **expected behavior** - the Android side is correctly rejecting connections that don't follow the full Nearby Sharing protocol.
