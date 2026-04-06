# Google Nearby Sharing & Connections for Linux (Unofficial)

<img width="982" height="760" alt="image" src="https://github.com/user-attachments/assets/9533ea09-81d9-4162-b90f-e0bd8b714d1d" />



## Demo


https://github.com/user-attachments/assets/048afa1e-40a4-4351-a859-c81b642fc6e3




This is an unofficial Linux-focused fork of Nearby.

## Purpose

The primary goal of this repository is to provide reusable Linux libraries/APIs for Nearby.

Nearby Share sample applications are included and will continue to be supported, but they are examples first and library integration is the main long-term target.

## Documentation

Technical deep dives are being moved to the project wiki so this README can stay focused on setup and status.

- Wiki: https://github.com/kidfromjupiter/nearby/wiki

## Requirements

To run this project on Linux, you need:
- BlueZ `5.86` or higher
- `NetworkManager`
- `Avahi`
- Qt (required for the tray UI app)

## Installation (End Users)

1. Download the latest release archive from the Releases page.
- https://github.com/kidfromjupiter/nearby/releases

2. Extract it:

```bash
tar -xzf nearby-file-share-linux-*.tar.gz
```

3. Install the application, shared library, and launcher:
   (run this from the extracted bundle directory)

```bash
./install_nearby_file_share.sh
```

4. Apply a temporary BlueZ service override (required for now).
   Create/edit the `bluetooth.service` override:

```bash
sudo systemctl edit bluetooth
```

Add:

```ini
[Service]
ExecStart=
ExecStart=/usr/local/libexec/bluetooth/bluetoothd -E --debug=src/plugin.c --noplugin=bap,bass,mcp,vcp,micp,ccp,csip,tmap,asha,midi
```

Then reload and restart:

```bash
sudo systemctl daemon-reload
sudo systemctl restart bluetooth
```

These plugins are currently disabled due to an ongoing likely upstream BlueZ bug:
`bap,bass,mcp,vcp,micp,ccp,csip,tmap,asha,midi`.
This workaround will be removed once the issue is fixed.

This installs to `~/.local` by default.

For system-wide installation:

```bash
./install_nearby_file_share.sh --system
```

The installer also installs:
- `nearby_qml_file_tray_app`
- `libnearby_sharing_api_shared.so`
- `.desktop` launcher entry
- launcher icon

## Build From Source

Build and install the Linux Nearby Sharing library:

```bash
./sharing/linux/install_nearby_sharing_service.sh
```

Build tray sample app:

```bash
cd sharing/linux/qml_tray_app
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DNEARBY_PREFIX=/usr/local
cmake --build build -j
./build/nearby_qml_file_tray_app
```

## Current Status and Compatibility

The Linux implementation currently works with the reverse-engineered certificate manager used in this project.

Compatibility can still break if Google changes certificate manager behavior or protocol details. That component is closed source, so upstream regressions may be hard to diagnose immediately.

## Test Scope and Known Limits

- Single-file sharing has been tested.
- Multi-transfer behavior in one app lifetime has not been fully validated.
- Current practical testing often restarts the app before a new transfer.

In this project, a "session" means one application lifetime (start to exit).

After one transfer, some endpoints/internal state can reset or change, and this is still being investigated.

## Known Issues

- Linux hotspot startup can be slow.
- Connecting to another device's hotspot can be slow on Linux.
- Android-initiated connection formation can be very slow.
- 5 GHz hotspot discovery and connection support is currently removed due to Intel Wi-Fi driver instability related to LAR behavior on Linux. There is no known fix yet; this may be re-enabled in a later release.

A likely area to investigate is connection negotiation paths that Linux may not handle optimally yet.

## Wi-Fi Direct

Wi-Fi Direct is theoretically possible, but not implemented yet.

Reason: NetworkManager does not natively support the Wi-Fi Direct Group Owner flow required here.

## Contributing

- General: `CONTRIBUTING.md`
- Linux-specific: `LINUX_CONTRIBUTING.md`

## License

Released under `LICENSE`.
