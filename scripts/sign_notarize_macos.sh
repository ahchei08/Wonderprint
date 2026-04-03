#!/bin/bash

set -euo pipefail

if [ "$#" -lt 2 ] || [ "$#" -gt 3 ]; then
    echo "Usage: $0 <app-path> <output-dmg> [volume-name]" >&2
    exit 1
fi

APP_PATH="$1"
OUTPUT_DMG="$2"
VOLUME_NAME="${3:-Wonderprint-Orca}"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PACKAGE_SCRIPT="$SCRIPT_DIR/package_macos_dmg.sh"
ENTITLEMENTS="$SCRIPT_DIR/disable_validation.entitlements"

CERT_BASE64="${MACOS_CERTIFICATE_P12_BASE64:-${LEGACY_MACOS_CERTIFICATE_P12_BASE64:-}}"
CERT_PASSWORD="${MACOS_CERTIFICATE_PASSWORD:-${LEGACY_MACOS_CERTIFICATE_PASSWORD:-}}"
APPLE_LOGIN="${APPLE_ID:-}"
APPLE_PASSWORD="${APPLE_APP_SPECIFIC_PASSWORD:-}"
APPLE_TEAM="${APPLE_TEAM_ID:-}"

if [ ! -d "$APP_PATH" ]; then
    echo "App bundle not found: $APP_PATH" >&2
    exit 1
fi

for required_var in CERT_BASE64 CERT_PASSWORD APPLE_LOGIN APPLE_PASSWORD APPLE_TEAM; do
    if [ -z "${!required_var}" ]; then
        echo "Missing required signing/notarization configuration: $required_var" >&2
        exit 1
    fi
done

RUN_TEMP_DIR="${RUNNER_TEMP:-$(mktemp -d "${TMPDIR:-/tmp}/wonderprint-sign.XXXXXX")}"
CERTIFICATE_PATH="$RUN_TEMP_DIR/build_certificate.p12"
KEYCHAIN_PATH="$RUN_TEMP_DIR/app-signing.keychain-db"
KEYCHAIN_PASSWORD="$CERT_PASSWORD"

cleanup() {
    security delete-keychain "$KEYCHAIN_PATH" >/dev/null 2>&1 || true
}
trap cleanup EXIT

echo -n "$CERT_BASE64" | base64 -d -o "$CERTIFICATE_PATH"

CERT_FILE_SIZE=$(wc -c < "$CERTIFICATE_PATH")
if [ "${CERT_FILE_SIZE}" -lt 1024 ]; then
    echo "Decoded certificate looks invalid: ${CERTIFICATE_PATH}" >&2
    exit 1
fi

security create-keychain -p "$KEYCHAIN_PASSWORD" "$KEYCHAIN_PATH"
security set-keychain-settings -lut 21600 "$KEYCHAIN_PATH"
security unlock-keychain -p "$KEYCHAIN_PASSWORD" "$KEYCHAIN_PATH"
security import "$CERTIFICATE_PATH" -P "$CERT_PASSWORD" -A -t cert -f pkcs12 -k "$KEYCHAIN_PATH"
security list-keychain -d user -s "$KEYCHAIN_PATH"
security set-key-partition-list -S apple-tool:,apple:,codesign: -s -k "$KEYCHAIN_PASSWORD" "$KEYCHAIN_PATH"

CERTIFICATE_ID="$(
    security find-identity -v -p codesigning "$KEYCHAIN_PATH" |
    grep "Developer ID Application" |
    awk '{print $2}' |
    head -n1
)"

if [ -z "$CERTIFICATE_ID" ]; then
    echo "No Developer ID Application certificate found in temporary keychain." >&2
    exit 1
fi

codesign \
    --deep \
    --force \
    --verbose \
    --options runtime \
    --timestamp \
    --entitlements "$ENTITLEMENTS" \
    --sign "$CERTIFICATE_ID" \
    "$APP_PATH"

"$PACKAGE_SCRIPT" "$APP_PATH" "$OUTPUT_DMG" "$VOLUME_NAME"

codesign \
    --force \
    --verbose \
    --timestamp \
    --sign "$CERTIFICATE_ID" \
    "$OUTPUT_DMG"

xcrun notarytool submit \
    "$OUTPUT_DMG" \
    --apple-id "$APPLE_LOGIN" \
    --team-id "$APPLE_TEAM" \
    --password "$APPLE_PASSWORD" \
    --wait

xcrun stapler staple "$OUTPUT_DMG"

echo "Signed and notarized DMG: $OUTPUT_DMG"
