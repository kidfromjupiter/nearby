# Nearby File Share Tray App

This folder contains the Qt/QML **FileShareTray** application — a system tray
app for file sharing via Nearby Sharing, wired to:

- `nearby::sharing::linux::NearbySharingApi`
- Send mode (discover nearby share targets + send file)
- Receive mode (incoming requests + accept/reject)
- Transfer status list (progress + transfer status)
- Persistent tray behavior (window close hides app to tray)
- File logging to `/tmp/nearby_qml_file_tray.log`

## Files

- `file_share_tray_main.cpp`: Qt app bootstrap + system tray behavior.
- `file_share_tray_controller.h/.cc`: QML-facing backend wrapper around Nearby Sharing.
- `FileShareTray.qml`: Top-level UI for the file share tray app.
- `components/`: Shared QML UI components used by `FileShareTray.qml`.
- `resources_file_share.qrc`: Embeds `FileShareTray.qml` and components.

## Runtime behavior

- Close button does **not** terminate the process; it hides to tray.
- Use tray icon menu to show/hide/quit.
- Mode `Send`:
  - Starts discovery.
  - Shows discovered share targets.
  - Sends the selected file to a chosen target.
- Mode `Receive`:
  - Starts advertising.
  - Shows pending incoming transfer requests.
  - Lets you accept/reject incoming requests.
- Transfers are shown with target, direction, status, and progress.
- Logs are appended to `/tmp/nearby_qml_file_tray.log`.

## Building

This CMake app links against the installed Nearby shared library and header:

- `libnearby_sharing_api_shared.so`
- `sharing/linux/nearby_sharing_api.h`

Install them first (repo root):

```bash
./sharing/linux/install_nearby_sharing_service.sh
```

Then build the app (from `sharing/linux/qml_tray_app`):

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DNEARBY_PREFIX=/usr/local
cmake --build build -j
```

## Bundle `libnearby_sharing_api_shared.so` with the app

From `sharing/linux/qml_tray_app`:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$PWD/dist"
cmake --build build -j
cmake --install build
```

Bundle output:

- `dist/bin/nearby_qml_file_tray_app`
- `dist/bin/libnearby_sharing_api_shared.so`

The app is installed with `INSTALL_RPATH=$ORIGIN`, so it resolves the Nearby
shared library from the same folder in the bundle.

## Build a distributable `.zip` (includes runtime dependencies)

From `sharing/linux/qml_tray_app`:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
cpack --config build/CPackConfig.cmake -G ZIP
```

Output:

- `build/nearby_qml_tray_app-Linux-x86_64.zip`

This zip is created from the CMake install tree and includes:

- `nearby_qml_file_tray_app`
- `libnearby_sharing_api_shared.so`
- Qt runtime libs/plugins/QML imports discovered by Qt deploy tooling
