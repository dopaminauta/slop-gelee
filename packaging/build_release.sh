#!/bin/bash
# build_release.sh, Empaqueta el release portable de slop-gelee
set -euo pipefail

cd "$(dirname "${BASH_SOURCE[0]}")/.."
ROOT="$(pwd)"
VERSION="1.0.0"
NAME="slop-gelee-v${VERSION}-linux-x86_64"
STAGE="/tmp/${NAME}"
PKG="/tmp/${NAME}.tar.gz"

echo "=== 1/4 building release ==="
cmake -B build-release -DCMAKE_BUILD_TYPE=Release > /dev/null
cmake --build build-release -j"$(nproc)" 2>&1 | tail -2
strip build-release/tegrarcm-gui 2>/dev/null || true

echo "=== 2/4 staging structure ==="
rm -rf "$STAGE"
mkdir -p "$STAGE"/{tools,payloads,udev,assets}
cp build-release/tegrarcm-gui "$STAGE/"
cp tools/launcher_rcm "$STAGE/tools/"
cp tools/intermezzo.bin "$STAGE/tools/"
cp payloads/hekate_ctcaer_6.5.3.bin "$STAGE/payloads/"
mkdir -p "$STAGE/payloads/tools"
cp payloads/tools/*.bin payloads/tools/*.rom "$STAGE/payloads/tools/"
cp udev/50-tegrarcm.rules "$STAGE/udev/"
cp assets/tegrarcm.svg assets/slop-gelee.svg "$STAGE/assets/"
cp README.md LICENSE "$STAGE/"
cp packaging/install.sh "$STAGE/"
chmod +x "$STAGE/install.sh"

echo "=== 3/4 packaging ==="
tar -czf "$PKG" -C /tmp "$NAME"
sha256sum "$PKG" | tee "$STAGE/../${NAME}.sha256" || true
SHA=$(sha256sum "$PKG" | cut -d' ' -f1)
echo "SHA256: $SHA"
du -h "$PKG"

echo "=== 4/4 contents ==="
tar -tzf "$PKG"
echo ""
echo "PACKAGE: $PKG"
echo "SHA: $SHA"
