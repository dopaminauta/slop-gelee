#!/bin/bash
# install.sh, Instalador de slop-gelee (TegraRcmGUI-Linux)
# Uso: ./install.sh [--with-udev]
#   --with-udev  instala tambien las reglas udev (pide sudo)
set -euo pipefail

APP_NAME="tegrarcm-gui"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "=== slop-gelee: installing into ~/.local ==="

# 1. binary
mkdir -p ~/.local/bin
install -m 0755 "$SCRIPT_DIR/tegrarcm-gui" ~/.local/bin/
echo "[+] binary -> ~/.local/bin/tegrarcm-gui"

# 2. injection engine (launcher_rcm + intermezzo) next to the binary
mkdir -p ~/.local/tools
install -m 0755 "$SCRIPT_DIR/tools/launcher_rcm" ~/.local/tools/
install -m 0644 "$SCRIPT_DIR/tools/intermezzo.bin" ~/.local/tools/
echo "[+] engine -> ~/.local/tools/ (launcher_rcm + intermezzo.bin)"

# 3. udev rules (copy; activation with --with-udev)
mkdir -p ~/.local/udev
install -m 0644 "$SCRIPT_DIR/udev/50-tegrarcm.rules" ~/.local/udev/
echo "[+] udev rules -> ~/.local/udev/"

# 4. sample payload
if [ -f "$SCRIPT_DIR/payloads/hekate_ctcaer_6.5.3.bin" ]; then
    mkdir -p ~/.local/share/tegrarcm/payloads ~/.local/share/tegrarcm/payloads/tools
    install -m 0644 "$SCRIPT_DIR/payloads/hekate_ctcaer_6.5.3.bin" ~/.local/share/tegrarcm/payloads/
    install -m 0644 "$SCRIPT_DIR/payloads/tools/"*.bin "$SCRIPT_DIR/payloads/tools/"*.rom ~/.local/share/tegrarcm/payloads/tools/ 2>/dev/null || true
    echo "[+] sample payload -> ~/.local/share/tegrarcm/payloads/"
fi

# 5. icon + menu entry
if [ -f "$SCRIPT_DIR/assets/slop-gelee.svg" ]; then
    mkdir -p ~/.local/share/icons/hicolor/scalable/apps
    install -m 0644 "$SCRIPT_DIR/assets/slop-gelee.svg" ~/.local/share/icons/hicolor/scalable/apps/
fi
mkdir -p ~/.local/share/applications
cat > ~/.local/share/applications/slop-gelee.desktop << EOF
[Desktop Entry]
Type=Application
Name=slop gelee
Comment=Fusée Gelée payload injector for Nintendo Switch (RCM mode)
Exec=tegrarcm-gui
Icon=slop-gelee
Terminal=false
Categories=Utility;System;Qt;
Keywords=switch;rcm;payload;hekate;fusee;slop;gelee;
EOF
kbuildsycoca6 2>/dev/null || true
echo "[+] .desktop + icon installed (slop gelee)"

# 6. Udev (opcional)
if [ "${1:-}" = "--with-udev" ]; then
    echo "[*] Installing udev rules in /etc/udev/rules.d/ (sudo)..."
    sudo cp "$SCRIPT_DIR/udev/50-tegrarcm.rules" /etc/udev/rules.d/
    sudo udevadm control --reload
    sudo udevadm trigger
    echo "[+] udev rules active"
fi

echo ""
echo "=== done. run: tegrarcm-gui ==="
echo "=== tip: connect the Switch in RCM freshly armed (RCM mode expires on its own) ==="
