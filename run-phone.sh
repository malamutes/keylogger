#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

PACKAGE="${APP_PACKAGE:-com.example.mouse}"
ACTIVITY="${APP_ACTIVITY:-.MainActivity}"
PORT="${APP_PORT:-8080}"
DEVICE="${1:-${ADB_DEVICE:-}}"

if ! command -v adb >/dev/null 2>&1; then
  echo "Error: adb not found in PATH." >&2
  exit 1
fi

if [[ ! -x "./gradlew" ]]; then
  echo "Error: ./gradlew not found or not executable." >&2
  exit 1
fi

if [[ -z "$DEVICE" ]]; then
  mapfile -t DEVICES < <(adb devices | awk 'NR > 1 && $2 == "device" { print $1 }')
  if (( ${#DEVICES[@]} == 0 )); then
    echo "Error: no connected adb device found." >&2
    exit 1
  fi
  if (( ${#DEVICES[@]} > 1 )); then
    echo "Error: multiple devices found. Pass one explicitly:" >&2
    printf '  %s\n' "${DEVICES[@]}" >&2
    echo "Usage: ./run-phone.sh <device-id>" >&2
    exit 1
  fi
  DEVICE="${DEVICES[0]}"
fi

echo "[1/3] Setting adb reverse on $DEVICE (tcp:$PORT -> tcp:$PORT)"
adb -s "$DEVICE" reverse "tcp:$PORT" "tcp:$PORT"

echo "[2/3] Building and installing debug APK"
./gradlew installDebug --no-daemon

echo "[3/3] Launching ${PACKAGE}/${ACTIVITY}"
adb -s "$DEVICE" shell am start -n "${PACKAGE}/${ACTIVITY}"

echo "Done."
