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

### Supported mediums (Linux)

<table>
  <thead>
    <tr>
      <th align="left" colspan="2">Legend</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td><ul><li>[x] </li></ul></td>
      <td>Supported.</td>
    </tr>
    <tr>
      <td><ul><li>[ ] </li></ul></td>
      <td>Support is possible, but not implemented.</td>
    </tr>
    <tr>
      <td></td>
      <td>Support is not possible or does not make sense.</td>
    </tr>
  </tbody>
</table>

| Mediums           | Advertising            | Scanning               | Data                   |
| :---------------- | :--------------------: | :--------------------: | :--------------------: |
| Bluetooth Classic |                        |                        | <ul><li>[x] </li></ul> |
| BLE (Fast)        | <ul><li>[x] </li></ul> | <ul><li>[x] </li></ul> |                        |
| BLE (GATT)        | <ul><li>[x] </li></ul> | <ul><li>[x] </li></ul> |                        |
| BLE (Extended)    |                        |                        |                        |
| BLE (L2CAP)       |                        |                        |                        |
| Wi-Fi LAN         | <ul><li>[x] </li></ul> | <ul><li>[x] </li></ul> | <ul><li>[x] </li></ul> |
| Wi-Fi Hotspot     |                        |                        | <ul><li>[x] </li></ul> |
| Wi-Fi Direct      |                        |                        | <ul><li>[x] </li></ul> |
| Wi-Fi Aware       |                        |                        |                        |
| WebRTC            |                        |                        |                        |
| NFC               |                        |                        |                        |
| USB               |                        |                        |                        |
| AWDL              |                        |                        |                        |

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
