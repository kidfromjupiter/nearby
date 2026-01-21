# Linux Contributor Guide

This guide is for contributors working on the Linux platform implementation in this repo. It focuses on local development workflow, platform layout, and where to make changes for new or missing platform features.

For general project contribution rules, see `CONTRIBUTING.md`.

## Recommended development environment

The fastest path to a consistent Linux development setup is to use the devcontainer at `.devcontainers/bazelclion` with CLion and Bazel.

Notes:

- Use CLion devcontainers.
- You do not have to use a devcontainer, but it avoids environment drift.
- The devcontainer mounts `/run/dbus`, and the Linux implementation primarily uses D-Bus for platform communication.
- Inside the devcontainer, install:
  - `libbluetooth-dev`
  - `libgtest-dev`
  - `gdb` (optional but helpful)

## CLion + Bazel setup

Once the Bazel plugin loads in CLion, run a Bazel sync from the toolbar (top-right). This can take a while on first run.

After the sync completes, you are ready to develop.

## Source layout for Linux

Linux platform implementation lives here:

- `internal/platform/implementation/linux`

Key entry points:

- Platform contract: `internal/platform/implementation/platform.h`
- Linux implementation: `internal/platform/implementation/linux/platform.cc`

The Linux implementation largely mirrors the Windows platform layout and behavior, so it is useful to compare with the Windows implementation for parity.

## What the platform layer does

The Linux platform layer provides abstractions over local network and Bluetooth hardware so Nearby Connections can perform radio operations. These abstractions are organized around Media and Mediums. Each Medium defines a set of functions that the platform must implement.

If you are adding a feature or fixing a missing capability:

- Start from `internal/platform/implementation/platform.h`.
- Follow the Medium definitions to understand the required interface.
- Implement or extend the Linux counterparts under `internal/platform/implementation/linux`.

## Architecture overview

### Nearby Connections

At a high level, Nearby Connections is a cross-platform peer-to-peer connectivity stack with these layers:

1) Public API and internal orchestration: session management, discovery/advertising, and payload handling.
2) Core transport logic: connection lifecycle, endpoint negotiation, and encryption/authentication workflows.
3) Platform abstraction layer: the platform contract in `internal/platform/implementation/platform.h` defines the interfaces the core uses for discovery, advertising, and data transfer.
4) Medium implementations: concrete implementations of BLE, Bluetooth Classic, and Wi-Fi-based transports.

The platform layer is the bridge between the core stack and OS-specific capabilities. If a Medium is missing or incomplete at the platform layer, the corresponding transport features will be unavailable in Nearby Connections.

### Linux implementation

The Linux implementation mirrors the platform contract and wires it to Linux facilities (primarily D-Bus and BlueZ for Bluetooth). The implementation is organized as follows:

- `internal/platform/implementation/platform.h`: platform interfaces and contract.
- `internal/platform/implementation/linux`: Linux-specific implementations of the platform contract and Mediums.

Typical data flow:

1) The Nearby Connections core requests an operation (advertise, discover, connect, or send data).
2) The Linux platform layer maps the request to the relevant Medium implementation.
3) The Medium invokes Linux facilities (D-Bus, BlueZ, or network sockets) to perform the operation.
4) Events and data are surfaced back through the platform interfaces to the core stack.

When adding a new capability, start by matching the required interface in `internal/platform/implementation/platform.h`, then implement it for Linux in `internal/platform/implementation/linux` in the same shape as other Mediums. Use the Windows platform as a reference for expected behavior and edge cases.

## About Nearby Sharing on Linux

Nearby Sharing builds on top of Nearby Connections. The Linux implementation still uses the same platform abstractions described above.

The example application for Nearby Sharing is located at:

- `sharing/linux`

## Nearby Connections examples

Example applications for Nearby Connections are located at:

- Walkie-talkie: `nearby/connection/walkietalkie`
- File share: `nearby/connections/file_share`

## CLion project visibility

The `.bazelproject` configuration limits which directories CLion shows by default. If you want all directories visible, change the project root setting to `.`.

Be aware that enabling all directories can significantly impact performance and is not recommended for most machines.
