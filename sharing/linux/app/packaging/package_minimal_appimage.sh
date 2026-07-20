#!/usr/bin/env bash
set -euo pipefail
shopt -s nullglob

readonly SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
readonly WORKSPACE_ROOT="$(realpath "$SCRIPT_DIR/../../../..")"
readonly BINARY="$WORKSPACE_ROOT/bazel-bin/sharing/linux/app/app"
readonly RUNFILES="$WORKSPACE_ROOT/bazel-bin/sharing/linux/app/app.runfiles"
readonly QT_ROOT="$RUNFILES/rules_qt++fetch+qt_linux_x86_64"
readonly QT_DISTRIBUTION="$(dirname -- "$(readlink -f -- "$QT_ROOT/lib")")"
readonly QML_TOOL="$QT_DISTRIBUTION/bin/qml"
readonly APPIMAGE_RUNFILES="$WORKSPACE_ROOT/bazel-bin/sharing/linux/app/appimage.runfiles"
readonly APPIMAGETOOL="$APPIMAGE_RUNFILES/+http_file+appimagetool_x86_64/file/appimagetool-x86_64.AppImage"
readonly RUNTIME="$APPIMAGE_RUNFILES/+http_file+appimage_runtime_x86_64/file/runtime-x86_64"
readonly OUTPUT_DIR="$WORKSPACE_ROOT/sharing/linux/dist-minimal"
readonly FINAL_APPDIR="$OUTPUT_DIR/QuickShare.AppDir"
readonly OUTPUT_APPIMAGE="$OUTPUT_DIR/QuickShare-x86_64.AppImage"
readonly STAGE="$(mktemp -d /tmp/nearby-minimal-appdir.XXXXXX)"
readonly APPDIR="$STAGE/QuickShare.AppDir"

cleanup() {
  rm -rf -- "$STAGE"
}
trap cleanup EXIT

for required in \
  "$BINARY" \
  "$QT_ROOT/lib/libQt6Core.so.6.8.3" \
  "$QML_TOOL" \
  "$APPIMAGETOOL" \
  "$RUNTIME"; do
  if [[ ! -e "$required" ]]; then
    echo "Required packaging input is missing: $required" >&2
    exit 1
  fi
done

if [[ -e "$FINAL_APPDIR" || -e "$OUTPUT_APPIMAGE" ]]; then
  echo "Output already exists; move or remove it first: $OUTPUT_DIR" >&2
  exit 1
fi

mkdir -p \
  "$APPDIR/usr/bin" \
  "$APPDIR/usr/lib" \
  "$APPDIR/usr/lib/qt6/plugins" \
  "$APPDIR/usr/lib/qt6/qml" \
  "$APPDIR/usr/share/applications" \
  "$APPDIR/usr/share/icons/hicolor/scalable/apps" \
  "$APPDIR/usr/share/metainfo"

install -m 0755 "$BINARY" "$APPDIR/usr/bin/quickshare"

copy_qt_library() {
  local name="$1"
  local files=("$QT_ROOT/lib/lib${name}.so"*)
  if ((${#files[@]} == 0)); then
    echo "Matching Qt library is missing: lib${name}.so*" >&2
    return 1
  fi
  cp -a -- "${files[@]}" "$APPDIR/usr/lib/"
}

readonly DIRECT_QT_LIBRARIES=(
  Qt6Core
  Qt6DBus
  Qt6Gui
  Qt6LabsPlatform
  Qt6Network
  Qt6OpenGL
  Qt6Qml
  Qt6QmlMeta
  Qt6QmlModels
  Qt6QmlWorkerScript
  Qt6Quick
  Qt6QuickControls2
  Qt6QuickLayouts
  Qt6QuickShapes
  Qt6QuickTemplates2
  Qt6Widgets
)

for name in "${DIRECT_QT_LIBRARIES[@]}"; do
  copy_qt_library "$name"
done

for name in icudata icui18n icuio icutest icutu icuuc; do
  soname="lib${name}.so.73"
  source="$QT_ROOT/lib/$soname"
  if [[ ! -e "$source" ]]; then
    echo "Matching ICU library is missing: $soname" >&2
    exit 1
  fi
  # The rules_qt ICU links point at Bazel's external cache with absolute
  # targets. Materialize each SONAME so the AppDir never contains those
  # developer-machine-only links.
  install -m 0644 "$(readlink -f -- "$source")" "$APPDIR/usr/lib/$soname"
done

# Copy only the QML module roots used by the application. Copying all of
# QtQuick also deploys unrelated multimedia, 3D, virtual-keyboard, and style
# modules, greatly expanding the native dependency closure.
copy_module_files() {
  local source="$1"
  local destination="$2"
  mkdir -p "$destination"
  find -L "$source" -maxdepth 1 -type f \
    -exec cp -aL -t "$destination" -- {} +
}

copy_module_files "$QT_ROOT/qml/QtQml" "$APPDIR/usr/lib/qt6/qml/QtQml"
cp -aL \
  "$QT_ROOT/qml/QtQml/Models" \
  "$QT_ROOT/qml/QtQml/WorkerScript" \
  "$APPDIR/usr/lib/qt6/qml/QtQml/"
cp -aL "$QT_ROOT/qml/QtCore" "$APPDIR/usr/lib/qt6/qml/"

copy_module_files "$QT_ROOT/qml/QtQuick" "$APPDIR/usr/lib/qt6/qml/QtQuick"
for module in Window Layouts Shapes Templates; do
  cp -aL \
    "$QT_ROOT/qml/QtQuick/$module" \
    "$APPDIR/usr/lib/qt6/qml/QtQuick/"
done

copy_module_files \
  "$QT_ROOT/qml/QtQuick/Controls" \
  "$APPDIR/usr/lib/qt6/qml/QtQuick/Controls"
cp -aL \
  "$QT_ROOT/qml/QtQuick/Controls/Basic" \
  "$QT_ROOT/qml/QtQuick/Controls/Fusion" \
  "$QT_ROOT/qml/QtQuick/Controls/impl" \
  "$APPDIR/usr/lib/qt6/qml/QtQuick/Controls/"

mkdir -p "$APPDIR/usr/lib/qt6/qml/Qt/labs"
cp -aL \
  "$QT_ROOT/qml/Qt/labs/platform" \
  "$APPDIR/usr/lib/qt6/qml/Qt/labs/"

# Deploy the XCB, Wayland, headless-test, and SVG plugins. Wayland client-side
# support is copied as a unit because the compositor selects its shell,
# decoration, and buffer integrations dynamically.
readonly PLUGIN_CATEGORIES=(
  platforms
  imageformats
  wayland-decoration-client
  wayland-graphics-integration-client
  wayland-shell-integration
)
for category in "${PLUGIN_CATEGORIES[@]}"; do
  mkdir -p "$APPDIR/usr/lib/qt6/plugins/$category"
done

for plugin in \
  libqoffscreen.so \
  libqxcb.so \
  libqwayland-egl.so \
  libqwayland-generic.so; do
  cp -aL \
    "$QT_ROOT/plugins/platforms/$plugin" \
    "$APPDIR/usr/lib/qt6/plugins/platforms/"
done

cp -aL \
  "$QT_ROOT/plugins/imageformats/libqsvg.so" \
  "$APPDIR/usr/lib/qt6/plugins/imageformats/"

for category in \
  wayland-decoration-client \
  wayland-graphics-integration-client \
  wayland-shell-integration; do
  cp -aL \
    "$QT_ROOT/plugins/$category/." \
    "$APPDIR/usr/lib/qt6/plugins/$category/"
done

# Close only the Qt/ICU portion of the ELF graph. Every non-Qt dependency is a
# target-system responsibility; in particular, never copy the build host's
# libcurl, C++ runtime, OpenSSL, D-Bus, graphics, desktop, or network stack.
while true; do
  added=0
  mapfile -t elf_files < <(find "$APPDIR" -type f -print | sort)

  for elf in "${elf_files[@]}"; do
    file -L "$elf" | grep -q ELF || continue

    while IFS= read -r soname; do
      [[ -n "$soname" ]] || continue
      [[ -e "$APPDIR/usr/lib/$soname" ]] && continue

      if [[ "$soname" == libQt6*.so* || "$soname" == libicu*.so* ]]; then
        source="$QT_ROOT/lib/$soname"
      else
        continue
      fi

      if [[ -z "$source" || ! -e "$source" ]]; then
        echo "Unable to resolve $soname, required by $elf" >&2
        exit 1
      fi

      echo "Bundling $soname"
      install -m 0644 "$(readlink -f -- "$source")" "$APPDIR/usr/lib/$soname"
      added=1
    done < <(
      readelf -d "$elf" 2>/dev/null |
        sed -n 's/.*Shared library: \[\(.*\)\]/\1/p'
    )
  done

  ((added == 0)) && break
done

install -m 0644 \
  "$WORKSPACE_ROOT/sharing/linux/app/packaging/quickshare.desktop" \
  "$APPDIR/usr/share/applications/quickshare.desktop"
install -m 0644 \
  "$WORKSPACE_ROOT/sharing/linux/app/packaging/quickshare.metainfo.xml" \
  "$APPDIR/usr/share/metainfo/quickshare.metainfo.xml"
install -m 0644 \
  "$WORKSPACE_ROOT/sharing/linux/app/icons/quickshare.svg" \
  "$APPDIR/usr/share/icons/hicolor/scalable/apps/quickshare.svg"

cp \
  "$APPDIR/usr/share/applications/quickshare.desktop" \
  "$APPDIR/"
cp \
  "$APPDIR/usr/share/icons/hicolor/scalable/apps/quickshare.svg" \
  "$APPDIR/"

printf '%s\n' \
  '[Paths]' \
  'Prefix=..' \
  'Plugins=lib/qt6/plugins' \
  'Qml2Imports=lib/qt6/qml' \
  >"$APPDIR/usr/bin/qt.conf"

printf '%s\n' \
  '#!/bin/sh' \
  'set -eu' \
  '' \
  'HERE=$(CDPATH= cd -- "$(dirname -- "$(readlink -f -- "$0")")" && pwd)' \
  '' \
  'export LD_LIBRARY_PATH="$HERE/usr/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"' \
  'export QT_PLUGIN_PATH="$HERE/usr/lib/qt6/plugins"' \
  'export QT_QPA_PLATFORM_PLUGIN_PATH="$HERE/usr/lib/qt6/plugins/platforms"' \
  'export QML_IMPORT_PATH="$HERE/usr/lib/qt6/qml"' \
  'export QML2_IMPORT_PATH="$HERE/usr/lib/qt6/qml"' \
  'export QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-wayland;xcb}"' \
  '' \
  'exec "$HERE/usr/bin/quickshare" "$@"' \
  >"$APPDIR/AppRun"
chmod 0755 "$APPDIR/AppRun"

# Fail on an absent dependency, a symbol-version mismatch, or any Qt library
# resolved from the host.
audit_failed=0
mapfile -t elf_files < <(find "$APPDIR" -type f -print | sort)
for elf in "${elf_files[@]}"; do
  file -L "$elf" | grep -q ELF || continue
  output="$(env LD_LIBRARY_PATH="$APPDIR/usr/lib" ldd "$elf" 2>&1 || true)"
  problems="$(grep -E 'not found|version `' <<<"$output" || true)"
  host_qt="$(grep -E 'libQt6.* => /(lib|usr/lib)' <<<"$output" || true)"

  if [[ -n "$problems" || -n "$host_qt" ]]; then
    echo "Dependency audit failed: $elf" >&2
    [[ -n "$problems" ]] && printf '%s\n' "$problems" >&2
    [[ -n "$host_qt" ]] && printf '%s\n' "$host_qt" >&2
    audit_failed=1
  fi
done
((audit_failed == 0)) || exit 1

while IFS= read -r -d '' link; do
  if [[ ! -e "$link" ]]; then
    echo "Broken packaged symlink: $link -> $(readlink -- "$link")" >&2
    audit_failed=1
  fi
done < <(find "$APPDIR" -type l -print0)
((audit_failed == 0)) || exit 1

# Validate the deployed QML import graph independently of the Nearby backend,
# which requires access to the system D-Bus and Bluetooth services. A qt.conf
# beside this temporary copy makes the QML tool use only the staged AppDir.
readonly QML_TEST_DIR="$STAGE/qml-test"
readonly QML_SMOKE_LOG="$STAGE/qml-smoke-test.log"
mkdir -p "$QML_TEST_DIR"
install -m 0755 "$QML_TOOL" "$QML_TEST_DIR/qml"
printf '%s\n' \
  '[Paths]' \
  "Prefix=$APPDIR/usr" \
  'Plugins=lib/qt6/plugins' \
  'Qml2Imports=lib/qt6/qml' \
  >"$QML_TEST_DIR/qt.conf"
printf '%s\n' \
  'import QtCore' \
  'import QtQuick' \
  'import QtQuick.Window' \
  'import QtQuick.Controls' \
  'import QtQuick.Layouts' \
  'import QtQuick.Shapes' \
  'import Qt.labs.platform as Labs' \
  'ApplicationWindow {' \
  '    visible: false' \
  '    Component.onCompleted: Qt.quit()' \
  '}' \
  >"$QML_TEST_DIR/smoke.qml"

set +e
env \
  LD_LIBRARY_PATH="$APPDIR/usr/lib:$QT_DISTRIBUTION/lib" \
  QT_QPA_PLATFORM=offscreen \
  QT_DEBUG_PLUGINS=1 \
  QML_IMPORT_TRACE=1 \
  timeout 15s "$QML_TEST_DIR/qml" \
  -I "$APPDIR/usr/lib/qt6/qml" \
  "$QML_TEST_DIR/smoke.qml" \
  >"$QML_SMOKE_LOG" 2>&1
qml_smoke_status=$?
set -e

if [[ "$qml_smoke_status" -ne 0 ]]; then
  echo "QML import smoke test failed with status $qml_smoke_status" >&2
  tail -n 100 "$QML_SMOKE_LOG" >&2
  exit "$qml_smoke_status"
fi

# The complete application smoke test is opt-in because its Nearby backend
# opens the system D-Bus before loading QML. Sandboxes without system-bus
# access abort here even when the package itself is valid.
if [[ "${RUN_BACKEND_SMOKE_TEST:-0}" == 1 ]]; then
  readonly BACKEND_SMOKE_LOG="$STAGE/backend-smoke-test.log"
  set +e
  env \
    QT_QPA_PLATFORM=offscreen \
    QT_DEBUG_PLUGINS=1 \
    QML_IMPORT_TRACE=1 \
    timeout 15s "$APPDIR/AppRun" \
    >"$BACKEND_SMOKE_LOG" 2>&1
  backend_smoke_status=$?
  set -e

  if [[ "$backend_smoke_status" -ne 0 && "$backend_smoke_status" -ne 124 ]]; then
    echo "Backend smoke test failed with status $backend_smoke_status" >&2
    tail -n 100 "$BACKEND_SMOKE_LOG" >&2
    exit "$backend_smoke_status"
  fi
fi

mkdir -p "$OUTPUT_DIR"
env \
  ARCH=x86_64 \
  APPIMAGE_EXTRACT_AND_RUN=1 \
  "$APPIMAGETOOL" \
  --runtime-file "$RUNTIME" \
  "$APPDIR" \
  "$OUTPUT_APPIMAGE"

cp -a "$APPDIR" "$FINAL_APPDIR"
cp "$QML_SMOKE_LOG" "$OUTPUT_DIR/qml-smoke-test.log"
if [[ -v BACKEND_SMOKE_LOG ]]; then
  cp "$BACKEND_SMOKE_LOG" "$OUTPUT_DIR/backend-smoke-test.log"
fi

echo "Created $OUTPUT_APPIMAGE"
echo "Retained $FINAL_APPDIR"
