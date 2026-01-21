# Unofficial Linux Nearby

This repository is an unofficial Linux implementation of Google Nearby, forked from the official Nearby codebase. It focuses on Linux platform support for Nearby Connections, Nearby Sharing, and Nearby Presence.

This is not an officially supported Google product.

## What is included

- Nearby Connections
- Nearby Sharing
- Nearby Presence

## Linux platform support

The Linux platform implementation provides abstraction layers over local networking and Bluetooth hardware to support Nearby radio operations. The current Linux implementation supports:

- BLE discovery and advertising
- GATT advertising and discovery
- Data transfer over Bluetooth Classic
- Wi-Fi LAN
- Wi-Fi Hotspot
- Wi-Fi Direct
- Wi-Fi LAN advertising and discovery

## Example applications

- Nearby Sharing service example: `sharing/linux`
- Nearby Connections examples:
  - Walkie-talkie: `nearby/connection/walkietalkie`
  - File share: `nearby/connections/file_share`

## Contributing

General contribution guidelines are in `CONTRIBUTING.md`.

If you are working on the Linux platform, start with `LINUX_CONTRIBUTING.md`.

## License

Nearby is released under the `LICENSE`.
