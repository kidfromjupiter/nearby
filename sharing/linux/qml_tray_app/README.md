# Nearby QML Tray App

This folder contains a Qt/QML tray application backend and UI wired to:

- `nearby::sharing::linux::NearbyConnectionsServiceLinux`
- Send mode (discovery + connect)
- Receive mode (incoming requests + accept/reject)
- Transfer status list (progress + payload status)
- Connected medium display per endpoint
- Persistent tray behavior (window close hides app to tray)
- File logging to `/tmp/nearby_qml_tray.log`

## Files

- `main.cpp`: Qt app bootstrap + system tray behavior.
- `nearby_tray_controller.h/.cc`: QML-facing backend wrapper around Nearby Connections.
- `Main.qml`: UI for send/receive workflows and transfer monitoring.
- `resources.qrc`: embeds `Main.qml`.

## Runtime behavior

- Close button does **not** terminate the process; it hides to tray.
- Use tray icon menu to show/hide/quit.
- Mode `Send`:
  - Starts discovery.
  - Shows discovered endpoints.
  - Lets you connect and send text payloads.
- Mode `Receive`:
  - Starts advertising.
  - Shows pending incoming connection requests.
  - Lets you accept/reject incoming requests.
- Transfers are shown with endpoint, direction, status, progress, and medium.
- Logs are appended to `/tmp/nearby_qml_tray.log`.

## Building (host stays clean)

This app now has a Bazel target:

- `//sharing/linux/qml_tray_app:nearby_qml_tray_app`

Qt compiler and linker flags are resolved through `pkg-config` at build time.

### Recommended: build in container

From repo root:

```bash
./sharing/linux/qml_tray_app/build_in_container.sh
```

This command:

- builds an image from `sharing/linux/qml_tray_app/container/Dockerfile`
- installs Bazel + Qt 6 dev packages inside that image
- runs the Bazel build in the container
- keeps Bazel cache in a Docker volume (`nearby_qml_tray_bazel_cache`)

Useful overrides:

```bash
# Use podman instead of docker.
CONTAINER_RUNTIME=podman ./sharing/linux/qml_tray_app/build_in_container.sh

# Pin a custom image tag.
QML_TRAY_IMAGE_TAG=nearby-qml-tray-builder:v1 ./sharing/linux/qml_tray_app/build_in_container.sh
```

### Build in an already-provisioned environment

If you already have Bazel + Qt 6 dev dependencies installed:

```bash
./sharing/linux/qml_tray_app/build.sh
```

## Bundle `libnearby_connections_service_linux_shared.so` with the app

From `sharing/linux/qml_tray_app`:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$PWD/dist"
cmake --build build -j
cmake --install build
```

Bundle output:

- `dist/bin/nearby_qml_tray_app`
- `dist/bin/libnearby_connections_service_linux_shared.so`

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

- `nearby_qml_tray_app`
- `libnearby_connections_service_linux_shared.so`
- Qt runtime libs/plugins/QML imports discovered by Qt deploy tooling
