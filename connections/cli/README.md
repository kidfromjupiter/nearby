# Nearby Connections CLI

`nearby_connections_cli` is a stateful command-line client for the classic
Nearby Connections endpoint API. It exposes every `Core` operation and every
public field in `AdvertisingOptions`, `DiscoveryOptions`, and
`ConnectionOptions`, including bytes, file, and stream payload variants.

Build and start it:

```sh
bazel build //connections/cli:nearby_connections_cli
bazel-bin/connections/cli/nearby_connections_cli
```

At the `nearby>` prompt, use `help` for commands and `help options` for the
complete option reference.

A simple two-device session looks like this:

```text
# Device A
nearby> advertise --service-id chat --endpoint-info Alice --strategy cluster --mediums ble,wifi-lan --auto-accept

# Device B
nearby> discover --service-id chat --strategy cluster --mediums ble,wifi-lan
# Wait for endpoint-found, then:
nearby> request ABCD --endpoint-info Bob --strategy cluster --mediums ble,wifi-lan --auto-accept
nearby> send-bytes ABCD "hello from Bob"
nearby> send-file ABCD "/tmp/photo with spaces.jpg"
```

Commands can also be automated:

```sh
bazel-bin/connections/cli/nearby_connections_cli \
  --execute 'discover --service-id chat --mediums ble' \
  --execute 'endpoint-id'
```

Use `--script FILE` to execute one command per line. Empty lines and lines
whose first token starts with `#` are ignored. Unless `--no-repl` is supplied,
the interactive prompt starts after scripted commands.

The V3 device API is intentionally not mixed into this executable. The
library's own `Core` documentation forbids mixing classic and V3 operations in
one client instance; `connections/file_share` is the existing V3 example.
