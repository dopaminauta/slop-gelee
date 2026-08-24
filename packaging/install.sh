#!/bin/bash
# install.sh — Instalador de slop-gelee (TegraRcmGUI-Linux)
# Uso: ./install.sh [--with-udev]
#   --with-udev  instala tambien las reglas udev (pide sudo)
set -euo pipefail

APP_NAME="tegrarcm-gui"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "=== slop-gelee: instalando en ~/.local ==="

# 1. Binario
mkdir -p ~/.local/bin
install -m 0755 "$SCRIPT_DIR/tegrarcm-gui" ~/.local/bin/
echo "[+] binario -> ~/.local/bin/tegrarcm-gui"

# 2. Motor de inyeccion (launcher_rcm + intermezzo) al lado del binario
mkdir -p ~/.local/tools
install -m 0755 "$SCRIPT_DIR/tools/launcher_rcm" ~/.local/tools/
install -m 0644 "$SCRIPT_DIR/tools/intermezzo.bin" ~/.local/tools/
echo "[+] motor -> ~/.local/tools/ (launcher_rcm + intermezzo.bin)"

# 3. Reglas udev (copia; activacion con --with-udev)
mkdir -p ~/.local/udev
install -m 0644 "$SCRIPT_DIR/udev/50-tegrarcm.rules" ~/.local/udev/
echo "[+] udev rules -> ~/.local/udev/"

# 4. Payload de ejemplo
if [ -f "$SCRIPT_DIR/payloads/hekate_ctcaer_6.5.3.bin" ]; then
    mkdir -p ~/.local/share/tegrarcm/payloads
    install -m 0644 "$SCRIPT_DIR/payloads/hekate_ctcaer_6.5.3.bin" ~/.local/share/tegrarcm/payloads/
    echo "[+] payload ejemplo -> ~/.local/share/tegrarcm/payloads/"
fi

# 5. Icono + entrada de menu
if [ -f "$SCRIPT_DIR/assets/tegrarcm.svg" ]; then
    mkdir -p ~/.local/share/icons/hicolor/scalable/apps
    install -m 0644 "$SCRIPT_DIR/assets/tegrarcm.svg" ~/.local/share/icons/hicolor/scalable/apps/
fi
mkdir -p ~/.local/share/applications
cat > ~/.local/share/applications/tegrarcm-gui.desktop << EOF
[Desktop Entry]
Type=Application
Name=TegraRcmGUI
Name[es]=TegraRcmGUI
Comment=Injectar payloads Fusée Gelée en Nintendo Switch (modo RCM)
Comment[es]=Inyecta payloads Fusée Gelée en Nintendo Switch (modo RCM)
Exec=tegrarcm-gui
Icon=tegrarcm
Terminal=false
Categories=Utility;System;Qt;
Keywords=switch;rcm;payload;hekate;fusee;
EOF
kbuildsycoca6 2>/dev/null || true
echo "[+] .desktop + icono instalados"

# 6. Udev (opcional)
if [ "${1:-}" = "--with-udev" ]; then
    echo "[*] Instalando reglas udev en /etc/udev/rules.d/ (sudo)..."
    sudo cp "$SCRIPT_DIR/udev/50-tegrarcm.rules" /etc/udev/rules.d/
    sudo udevadm control --reload
    sudo udevadm trigger
    echo "[+] reglas udev activas"
fi

echo ""
echo "=== Listo. Abri con: tegrarcm-gui ==="
echo "=== Tip: conecta la Switch en RCM RECIEN armada (el modo RCM se agota solo) ==="
