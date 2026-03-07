#!/usr/bin/env bash

set -euo pipefail

MODE="user"
PREFIX=""

usage() {
  cat <<'USAGE'
Usage: ./install_nearby_file_share.sh [options]

Install Nearby File Share app artifacts from an extracted release bundle.

Options:
  --user          Install under $HOME/.local (default)
  --system        Install under /usr/local (uses sudo when needed)
  --prefix DIR    Custom install prefix (overrides --user/--system default)
  -h, --help      Show this help

Examples:
  ./install_nearby_file_share.sh
  ./install_nearby_file_share.sh --system
  ./install_nearby_file_share.sh --prefix "$HOME/.local"
USAGE
}

nearest_existing_parent() {
  local path="$1"
  while [[ ! -e "$path" ]]; do
    path="$(dirname "$path")"
  done
  printf '%s\n' "$path"
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --user)
      MODE="user"
      shift
      ;;
    --system)
      MODE="system"
      shift
      ;;
    --prefix)
      PREFIX="$2"
      shift 2
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

if [[ -z "$PREFIX" ]]; then
  if [[ "$MODE" == "system" ]]; then
    PREFIX="/usr/local"
  else
    PREFIX="$HOME/.local"
  fi
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

BIN_SRC="$SCRIPT_DIR/bin/nearby_qml_file_tray_app"
LIB_SRC="$SCRIPT_DIR/lib/libnearby_sharing_api_shared.so"
DESKTOP_SRC="$SCRIPT_DIR/share/applications/nearby-file-share.desktop"
ICON_SRC="$SCRIPT_DIR/share/icons/hicolor/256x256/apps/nearby-file-share.png"
HEADER_SRC="$SCRIPT_DIR/include/sharing/linux/nearby_sharing_api.h"

for required in "$BIN_SRC" "$LIB_SRC" "$DESKTOP_SRC" "$ICON_SRC"; do
  if [[ ! -f "$required" ]]; then
    echo "Missing required bundle artifact: $required" >&2
    echo "Run this installer from the extracted release bundle root." >&2
    exit 1
  fi
done

BINDIR="$PREFIX/bin"
LIBDIR="$PREFIX/lib"
INCLUDEDIR="$PREFIX/include/sharing/linux"
DESKTOP_DIR="$PREFIX/share/applications"
ICON_DIR="$PREFIX/share/icons/hicolor/256x256/apps"

NEEDS_ELEVATION=0
for path in "$BINDIR" "$LIBDIR" "$DESKTOP_DIR" "$ICON_DIR"; do
  parent="$(nearest_existing_parent "$path")"
  if [[ ! -w "$parent" ]]; then
    NEEDS_ELEVATION=1
    break
  fi
done

INSTALL_PREFIX=()
if [[ "$NEEDS_ELEVATION" -eq 1 && "$(id -u)" -ne 0 ]]; then
  if ! command -v sudo >/dev/null 2>&1; then
    echo "Install requires elevated privileges, but sudo is not available." >&2
    exit 1
  fi
  INSTALL_PREFIX=(sudo)
fi

TMP_DESKTOP="$(mktemp)"
trap 'rm -f "$TMP_DESKTOP"' EXIT

sed \
  -e "s|^Exec=.*|Exec=${BINDIR}/nearby_qml_file_tray_app|" \
  -e "s|^Icon=.*|Icon=${ICON_DIR}/nearby-file-share.png|" \
  "$DESKTOP_SRC" > "$TMP_DESKTOP"

echo "[1/5] Installing application binary"
"${INSTALL_PREFIX[@]}" install -d "$BINDIR"
"${INSTALL_PREFIX[@]}" install -m 0755 "$BIN_SRC" "$BINDIR/"

echo "[2/5] Installing shared library"
"${INSTALL_PREFIX[@]}" install -d "$LIBDIR"
"${INSTALL_PREFIX[@]}" install -m 0755 "$LIB_SRC" "$LIBDIR/"

if [[ -f "$HEADER_SRC" ]]; then
  echo "[3/5] Installing public header"
  "${INSTALL_PREFIX[@]}" install -d "$INCLUDEDIR"
  "${INSTALL_PREFIX[@]}" install -m 0644 "$HEADER_SRC" "$INCLUDEDIR/"
else
  echo "[3/5] Header not present in bundle; skipping"
fi

echo "[4/5] Installing desktop entry and icon"
"${INSTALL_PREFIX[@]}" install -d "$DESKTOP_DIR"
"${INSTALL_PREFIX[@]}" install -d "$ICON_DIR"
"${INSTALL_PREFIX[@]}" install -m 0644 "$TMP_DESKTOP" "$DESKTOP_DIR/nearby-file-share.desktop"
"${INSTALL_PREFIX[@]}" install -m 0644 "$ICON_SRC" "$ICON_DIR/nearby-file-share.png"

echo "[5/5] Refreshing desktop/system caches"
if command -v update-desktop-database >/dev/null 2>&1; then
  "${INSTALL_PREFIX[@]}" update-desktop-database "$DESKTOP_DIR" || true
fi
if command -v ldconfig >/dev/null 2>&1 && [[ "$PREFIX" == "/usr" || "$PREFIX" == "/usr/local" ]]; then
  "${INSTALL_PREFIX[@]}" ldconfig || true
fi

echo "Installed Nearby File Share:"
echo "  binary : $BINDIR/nearby_qml_file_tray_app"
echo "  library: $LIBDIR/libnearby_sharing_api_shared.so"
echo "  desktop: $DESKTOP_DIR/nearby-file-share.desktop"
echo "  icon   : $ICON_DIR/nearby-file-share.png"
if [[ -f "$HEADER_SRC" ]]; then
  echo "  header : $INCLUDEDIR/nearby_sharing_api.h"
fi
