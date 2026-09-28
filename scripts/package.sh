#!/usr/bin/env bash
# Builds a self-contained, ad-hoc signed Hangar.app + Hangar.dmg.
#   ./scripts/package.sh            build only
#   ./scripts/package.sh --install  build and install into /Applications
set -euo pipefail
cd "$(dirname "$0")/.."

QT="$(brew --prefix qt)"
BREW="$(brew --prefix)"
OUT=build-release
APP="$OUT/Hangar.app"

# 1. App icon: .icns from the 1024px PNG with Apple's own tools.
if [ ! -f resources/AppIcon.icns ] || [ resources/AppIcon.png -nt resources/AppIcon.icns ]; then
    ICONSET="$(mktemp -d)/AppIcon.iconset"
    mkdir -p "$ICONSET"
    for s in 16 32 128 256 512; do
        sips -z $s $s resources/AppIcon.png --out "$ICONSET/icon_${s}x${s}.png" >/dev/null
        sips -z $((s*2)) $((s*2)) resources/AppIcon.png --out "$ICONSET/icon_${s}x${s}@2x.png" >/dev/null
    done
    iconutil -c icns "$ICONSET" -o resources/AppIcon.icns
fi

# 2. Clean bundle BEFORE configuring: CMake writes Contents/Info.plist at
#    configure time, and macdeployqt is not idempotent.
rm -rf "$APP" "$OUT/Hangar.dmg"
cmake -S . -B "$OUT" -G Ninja -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_PREFIX_PATH="$QT" -DHANGAR_BUILD_TESTS=OFF
cmake --build "$OUT"
test -f "$APP/Contents/Info.plist" || { echo "Info.plist missing"; exit 1; }

# 3. Bundle Qt. Homebrew splits Qt into kegs linked via @rpath.
"$QT/bin/macdeployqt" "$APP" -qmldir=qml -libpath="$QT/lib" -libpath="$BREW/lib" -no-strip

# 4. Re-sign after install_name_tool rewrote the libraries.
codesign --force --deep --sign - "$APP"
codesign --verify --deep --strict "$APP"

hdiutil create -volname Hangar -srcfolder "$APP" -ov -format UDZO "$OUT/Hangar.dmg" >/dev/null
echo "Built: $APP and $OUT/Hangar.dmg"

if [ "${1:-}" = "--install" ]; then
    pkill -x Hangar || true
    rm -rf /Applications/Hangar.app
    cp -R "$APP" /Applications/
    touch /Applications/Hangar.app
    /System/Library/Frameworks/CoreServices.framework/Frameworks/LaunchServices.framework/Support/lsregister -f /Applications/Hangar.app
    open /Applications/Hangar.app
    echo "Installed to /Applications/Hangar.app"
fi
