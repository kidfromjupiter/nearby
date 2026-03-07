# Nearby Sharing for Linux

This directory contains:
- A Linux-facing Nearby Sharing library (`NearbySharingApi`)
- A sample CLI app (`nearby_sharing_app`)
- A Qt/QML tray sample app (`qml_tray_app`)

## Scope and Intended Use

This project is primarily intended to be used as a reusable library/API.

The sample applications are still supported and will continue to be supported because they are used in real day-to-day device sharing workflows.

## Current Status

The current implementation works with the reverse-engineered certificate manager currently used in this project.

Compatibility can still break if Google changes certificate manager behavior/protocol details. That component is closed source, so upstream changes can be difficult to inspect and adapt to quickly.

## Test Coverage and Session Notes

- Verified: single-file sharing flow.
- Not fully verified: multiple transfers in one app lifetime.
- Current practical testing pattern: restart the application before each new transfer.

For this README, a "session" means one process lifetime (app start to app exit).

After one transfer, some endpoints may close and internal state can reset/change. Multi-transfer handling in one live session is still under investigation.

## Known Issues

- Linux hotspot startup can be slow.
- Connecting to a hotspot started on another device can be slow on Linux.
- Android-initiated connection formation can be very slow.

The Android/Linux connection latency issue still needs deeper investigation. One possible cause is connection/negotiation behavior that Linux does not currently handle well.

## Wi-Fi Direct Status

Wi-Fi Direct is theoretically possible but not implemented yet.

Reason: NetworkManager does not natively support creating Wi-Fi Direct Group Owners in the way this project needs.

## Installation

### 1. Install the shared library

From the repository root:

```bash
./sharing/linux/install_nearby_sharing_service.sh
```

This installs:
- `libnearby_sharing_api_shared.so`
- `sharing/linux/nearby_sharing_api.h`

### 2. Build and run the CLI sample app (optional)

From the repository root:

```bash
bazel build //sharing/linux:nearby_sharing_app
./bazel-bin/sharing/linux/nearby_sharing_app
# Optional custom device name
./bazel-bin/sharing/linux/nearby_sharing_app "MyDeviceName"
```

### 3. Build and run the tray sample app (optional)

From `sharing/linux/qml_tray_app`:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DNEARBY_PREFIX=/usr/local
cmake --build build -j
./build/nearby_qml_file_tray_app
```

### 4. Install launcher entry (`.desktop`) for the tray app

From `sharing/linux/qml_tray_app`:

```bash
mkdir -p "$HOME/.local/share/applications"
install -m 0644 nearby-file-share.desktop "$HOME/.local/share/applications/nearby-file-share.desktop"
sed -i "s|^Exec=.*|Exec=$(pwd)/build/nearby_qml_file_tray_app|" "$HOME/.local/share/applications/nearby-file-share.desktop"
sed -i "s|^Icon=.*|Icon=$(pwd)/tray_icon.png|" "$HOME/.local/share/applications/nearby-file-share.desktop"
update-desktop-database "$HOME/.local/share/applications" 2>/dev/null || true
```

After this, search for `Nearby File Share` in your desktop launcher.

## Documentation

Technical deep dives are being moved from README content to the wiki.

- Wiki: https://github.com/kidfromjupiter/nearby/wiki

This README stays focused on status, installation, and known limitations.

## Demo Assets (Planned)

- Video demo of end-to-end sharing flow.
- GIF showing the sharing process.

