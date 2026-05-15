#!/usr/bin/env bash
set -euo pipefail

# create_macos_dmg.sh
# Builds, installs, deploys Qt, optionally bundles LibreOffice.app, and creates a DMG.
#
# Usage:
#   ./installers/create_macos_dmg.sh \
#     --preset mac-release \
#     --build-dir out/build/macos/mac-release \
#     --install-dir out/install/macos/mac-release \
#     --output out/dist/zconverter-setup.dmg \
#     [--libreoffice /Applications/LibreOffice.app]

PRESET="mac-release"
BUILD_DIR="out/build/macos/mac-release"
INSTALL_DIR="out/install/macos/mac-release"
OUTPUT_DMG="out/dist/zconverter-setup.dmg"
LIBREOFFICE_APP=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        --preset)
            PRESET="$2"
            shift 2
            ;;
        --build-dir)
            BUILD_DIR="$2"
            shift 2
            ;;
        --install-dir)
            INSTALL_DIR="$2"
            shift 2
            ;;
        --output)
            OUTPUT_DMG="$2"
            shift 2
            ;;
        --libreoffice)
            LIBREOFFICE_APP="$2"
            shift 2
            ;;
        *)
            echo "Unknown argument: $1" >&2
            exit 2
            ;;
    esac
done

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

if [[ -z "$LIBREOFFICE_APP" && -d "/Applications/LibreOffice.app" ]]; then
    LIBREOFFICE_APP="/Applications/LibreOffice.app"
fi

mkdir -p "$(dirname "$OUTPUT_DMG")"

cmake --preset "$PRESET"
cmake --build "$BUILD_DIR" --config Release --target install

APP_BUNDLE="$INSTALL_DIR/zconverter.app"
if [[ ! -d "$APP_BUNDLE" ]]; then
    echo "App bundle not found: $APP_BUNDLE" >&2
    exit 3
fi

MACDEPLOYQT="$(command -v macdeployqt || true)"
if [[ -z "$MACDEPLOYQT" ]]; then
    echo "macdeployqt not found in PATH" >&2
    exit 4
fi

echo "Deploying Qt runtime into bundle"
"$MACDEPLOYQT" "$APP_BUNDLE" -always-overwrite -no-codesign

if [[ -n "$LIBREOFFICE_APP" ]]; then
    if [[ -d "$LIBREOFFICE_APP" ]]; then
        echo "Bundling LibreOffice.app: $LIBREOFFICE_APP"
        mkdir -p "$APP_BUNDLE/Contents/Frameworks"
        rm -rf "$APP_BUNDLE/Contents/Frameworks/LibreOffice.app"
        cp -R "$LIBREOFFICE_APP" "$APP_BUNDLE/Contents/Frameworks/LibreOffice.app"
    else
        echo "LibreOffice app not found: $LIBREOFFICE_APP" >&2
        exit 5
    fi
fi

STAGING_DIR="$(mktemp -d /tmp/zconverter-dmg.XXXXXX)"
cp -R "$APP_BUNDLE" "$STAGING_DIR/"
ln -s /Applications "$STAGING_DIR/Applications"

DMG_VOLUME_NAME="zconverter"
rm -f "$OUTPUT_DMG"
hdiutil create \
    -volname "$DMG_VOLUME_NAME" \
    -srcfolder "$STAGING_DIR" \
    -ov \
    -format UDZO \
    "$OUTPUT_DMG"

rm -rf "$STAGING_DIR"

echo "Created DMG: $OUTPUT_DMG"
