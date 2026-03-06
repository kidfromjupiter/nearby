#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DELEGATE_SCRIPT="${SCRIPT_DIR}/install_nearby_sharing_service.sh"

if [[ ! -x "${DELEGATE_SCRIPT}" ]]; then
  echo "Missing installer: ${DELEGATE_SCRIPT}" >&2
  exit 1
fi

cat <<'MSG'
install_nearby_connections_service.sh is deprecated.
Installing Nearby Sharing API artifacts instead.
MSG

exec "${DELEGATE_SCRIPT}" "$@"
