# QuickShare for Linux

## Native application logs

The QuickShare GUI writes its native Abseil diagnostics to:

```text
$XDG_STATE_HOME/quickshare/logs/quickshare.log
```

If `XDG_STATE_HOME` is not set to an absolute path, the directory defaults to
`$HOME/.local/state/quickshare/logs`.

Records are stored exactly as Abseil emits them. The active log rotates at
10 MiB, with `quickshare.log.1` through `quickshare.log.4` retaining the four
previous files. The directory and its files are accessible only to the current
user.

After file logging initializes, native Abseil diagnostics are not duplicated
to stderr. If the file cannot be initialized or written, diagnostics fall back
to stderr. Qt and QML messages keep their existing behavior and are not written
to these files.
