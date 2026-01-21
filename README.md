# Unofficial Linux Nearby

This repository is an unofficial Linux implementation of Google Nearby, forked from the official Nearby codebase. It focuses on Linux platform support for Nearby Connections, Nearby Sharing, and Nearby Presence.

Status: This is still a work in progress, and there are known bugs and issues that need to be sorted out.

Motivation: Google has multiple Linux implementation PRs open, but they have not been merged into the official repo. I am tired of begging maintainers, so I am maintaining this Linux-focused fork.

If you spot bugs, help identifying them is appreciated. Filing issues is always welcome. If you want to contribute, please start a discussion about the guidance you need.

Bazel is a terrible build system when it does not work and a pretty good one when it does. There is no in-between. The source code is unwieldy and it took me months to understand what everything does and where things go. It does not help that this is how I decided I am going to learn C++. Blessed be my naive soul.

## Testing status

I have tested with various setups:

- Android walkie-talkie example app detects and connects over Bluetooth Classic and Wi-Fi LAN and can share data. Medium upgrade is untested or unclear.
- Linux to Linux sharing is tested extensively so I can control the advertising and discovery environment.
- I do not have access to the Android implementation details.
- If you enable Quick Share visibility to Everyone on Android and run `sharing/linux` in advertising mode, it appears as a target in the Quick Share menu (promising, but not a full end-to-end validation).

What is tested:

- Linux to Linux sharing: advertising, discovery, and data work over Wi-Fi Direct, Wi-Fi Hotspot, Wi-Fi LAN, and Bluetooth Classic.
- BLE advertising and discovery work, but without a data medium.
- GATT advertising and discovery work.
- Android to Linux Nearby Connections work for the same mediums as Linux to Linux above (Bluetooth Classic, Wi-Fi LAN, Wi-Fi Hotspot, Wi-Fi Direct).

What is not tested or unknown:

- Android to Linux Nearby Sharing with Quick Share end-to-end.

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
| Bluetooth Classic | <ul><li>[x] </li></ul> | <ul><li>[x] </li></ul> | <ul><li>[x] </li></ul> |
| BLE (Fast)        |                        | <ul><li>[x] </li></ul> |                        |
| BLE (GATT)        | <ul><li>[x] </li></ul> | <ul><li>[x] </li></ul> |                        |
| BLE (Extended)    | <ul><li>[x] </li></ul> | <ul><li>[x] </li></ul> |                        |
| BLE (L2CAP)       |                        |                        |                        |
| Wi-Fi LAN         | <ul><li>[x] </li></ul> | <ul><li>[x] </li></ul> | <ul><li>[x] </li></ul> |
| Wi-Fi Hotspot     |                        |                        | <ul><li>[x] </li></ul> |
| Wi-Fi Direct      |                        |                        | <ul><li>[x] </li></ul> |
| Wi-Fi Aware       |                        |                        |                        |
| WebRTC            |                        |                        |                        |
| NFC               |                        |                        |                        |
| USB               |                        |                        |                        |
| AWDL              |                        |                        |                        |

Note: BLE fast advertising is not supported on Linux because Android uses separate LE random/resolvable MAC addresses per advertisement, and Linux does not currently support that. The Linux implementation uses BLE Extended advertising instead.

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
