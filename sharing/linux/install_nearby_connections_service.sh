#!/usr/bin/env bash

set -euo pipefail

TARGET="//sharing/linux:nearby_connections_service_linux_shared"
HEADER_SRC="sharing/linux/nearby_connections_qt_facade.h"
BAZEL_CMD="${BAZEL:-bazel}"
PREFIX="/usr/local"
LIBDIR=""
INCLUDEDIR=""
SKIP_BUILD=0
NEEDS_ELEVATION=0
INSTALL_PREFIX=()

usage() {
  cat <<USAGE
Usage: $0 [options]

Builds and installs the Nearby Connections shared library and public header.

Options:
  --prefix DIR       Install prefix (default: /usr/local)
  --libdir DIR       Library directory (default: <prefix>/lib)
  --includedir DIR   Include root directory (default: <prefix>/include)
  --bazel CMD        Bazel command (default: bazel or env BAZEL)
  --skip-build       Skip bazel build step and only install from bazel-bin
  -h, --help         Show this help

Examples:
  $0
  sudo $0
  sudo $0 --prefix /usr
  sudo $0 --bazel /usr/bin/bazel
USAGE
}

run_bazel() {
  # If invoked through sudo, run Bazel as the original user so Bazelisk reuses
  # that user's cache and does not re-download Bazel as root.
  if [[ "$(id -u)" -eq 0 && -n "${SUDO_USER:-}" ]]; then
    local caller_home
    caller_home="$(getent passwd "$SUDO_USER" | cut -d: -f6)"
    if [[ -z "$caller_home" ]]; then
      echo "Failed to resolve home directory for SUDO_USER=$SUDO_USER" >&2
      exit 1
    fi
    sudo -u "$SUDO_USER" -H env \
      HOME="$caller_home" \
      BAZELISK_HOME="${BAZELISK_HOME:-$caller_home/.cache/bazelisk}" \
      "$BAZEL_CMD" "$@"
  else
    "$BAZEL_CMD" "$@"
  fi
}

nearest_existing_parent() {
  local p="$1"
  while [[ ! -e "$p" ]]; do
    p="$(dirname "$p")"
  done
  printf '%s\n' "$p"
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --prefix)
      PREFIX="$2"
      shift 2
      ;;
    --libdir)
      LIBDIR="$2"
      shift 2
      ;;
    --includedir)
      INCLUDEDIR="$2"
      shift 2
      ;;
    --bazel)
      BAZEL_CMD="$2"
      shift 2
      ;;
    --skip-build)
      SKIP_BUILD=1
      shift
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "Unknown argument: $1" >&2
      usage
      exit 1
      ;;
  esac
done

if [[ -z "$LIBDIR" ]]; then
  LIBDIR="${PREFIX}/lib"
fi

if [[ -z "$INCLUDEDIR" ]]; then
  INCLUDEDIR="${PREFIX}/include"
fi

LIB_PARENT="$(nearest_existing_parent "$LIBDIR")"
INCLUDE_PARENT="$(nearest_existing_parent "${INCLUDEDIR}/sharing/linux")"

if [[ ! -w "$LIB_PARENT" || ! -w "$INCLUDE_PARENT" ]]; then
  NEEDS_ELEVATION=1
fi

if [[ "$NEEDS_ELEVATION" -eq 1 && "$(id -u)" -ne 0 ]]; then
  if ! command -v sudo >/dev/null 2>&1; then
    echo "Install requires elevated privileges, but sudo is not available." >&2
    exit 1
  fi
  INSTALL_PREFIX=(sudo)
fi

if [[ ! -f "$HEADER_SRC" ]]; then
  echo "Header not found: $HEADER_SRC" >&2
  echo "Run this script from the workspace root." >&2
  exit 1
fi

if [[ "$SKIP_BUILD" -eq 0 ]]; then
  echo "[1/4] Building $TARGET"
  run_bazel build "$TARGET"
else
  echo "[1/4] Skipping build (--skip-build)"
fi

echo "[2/4] Resolving bazel-bin path"
BAZEL_BIN="$(run_bazel info bazel-bin)"
LIB_SRC="${BAZEL_BIN}/sharing/linux/libnearby_connections_service_linux_shared.so"

if [[ ! -f "$LIB_SRC" ]]; then
  echo "Shared library not found: $LIB_SRC" >&2
  echo "Expected Bazel output for $TARGET is missing." >&2
  exit 1
fi

echo "[3/4] Installing library and header"
"${INSTALL_PREFIX[@]}" install -d "$LIBDIR"
"${INSTALL_PREFIX[@]}" install -d "${INCLUDEDIR}/sharing/linux"
"${INSTALL_PREFIX[@]}" install -m 0755 "$LIB_SRC" "$LIBDIR/"
"${INSTALL_PREFIX[@]}" install -m 0644 "$HEADER_SRC" "${INCLUDEDIR}/sharing/linux/"

if command -v ldconfig >/dev/null 2>&1; then
  echo "[4/4] Refreshing dynamic linker cache"
  "${INSTALL_PREFIX[@]}" ldconfig
else
  echo "[4/4] ldconfig not found; skipping linker cache refresh"
fi

echo "Installed:"
echo "  library: $LIBDIR/$(basename "$LIB_SRC")"
echo "  header : ${INCLUDEDIR}/sharing/linux/$(basename "$HEADER_SRC")"
