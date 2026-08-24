![GitHub](https://img.shields.io/github/license/eliboa/TegraRcmGUI)

# TegraRcmGUI-Linux

Port a Linux (Qt6 + libusb) de [TegraRcmGUI](https://github.com/eliboa/TegraRcmGUI), una GUI en C++ para
cargar *payloads* de recuperación (Fusée Gelée) en una consola Nintendo Switch por USB, en modo RCM
(Recovery Mode). El original es Windows-only (MFC/Win32 + libusbk); este port reemplaza esa capa por
Qt6 Widgets y libusb-1.0, conservando el resto de las features.

> Nota de encuadre: este es un port de plataforma de una herramienta open-source (GPL-2.0) de
> interoperabilidad de hardware propio (homebrew). El equivalente sería portar un flasher de BIOS o un
> loader de recovery. Solo funciona en unidades Switch "unpatched" (fabricadas antes de julio de 2018) —
> comprobalo en https://ismyswitchpatched.com/

## Dependencias (Arch / CachyOS)

```
sudo pacman -S qt6-base qt6-svg libusb python-pyusb
```

- **qt6-base** — Qt6 Widgets (interfaz)
- **qt6-svg** — motor SVG de Qt, necesario para el ícono de la app (`assets/tegrarcm.svg`)
- **libusb** — capa USB de la aplicación (detección de dispositivo en modo RCM)
- **launcher_rcm** — motor de inyección nativo en C (incluido en el repo y en el release), réplica del
  motor de NXLoader corregida contra el PoC oficial de ktemkin (recipient ENDPOINT + submit-only URB)
- **pkexec** (parte de `polkit`, normalmente ya instalado en un entorno de escritorio) — usado por el botón
  "Instalar reglas udev" de la pestaña Settings, para pedir autenticación gráfica

CMake, un compilador C++17 y `pkg-config` son necesarios para compilar (`base-devel`, `cmake`).

## Compilar

```
cmake -B build && cmake --build build -j$(nproc)
```

Esto genera el binario `build/tegrarcm-gui`. Podés correrlo directamente desde ahí — busca
`tools/launcher_rcm`, `tools/intermezzo.bin` y `udev/50-tegrarcm.rules` como hermanos de la raíz del repo
(ver sección "Estructura de rutas" abajo), así que **no** hace falta instalar nada para probarlo en el árbol
de build.

## Instalar

1. Copiar el binario:
   ```
   mkdir -p ~/.local/bin
   cp build/tegrarcm-gui ~/.local/bin/
   ```
2. Copiar el motor de inyección **al lado** de `bin/`, no dentro (la app busca `../tools/launcher_rcm`
   y `../tools/intermezzo.bin` relativo al binario):
   ```
   mkdir -p ~/.local/tools
   cp tools/launcher_rcm tools/intermezzo.bin ~/.local/tools/
   ```
3. (Opcional) copiar las reglas udev con el mismo criterio, para que el botón "Instalar reglas udev" las
   encuentre sin tener que apuntar al repo:
   ```
   mkdir -p ~/.local/udev
   cp udev/50-tegrarcm.rules ~/.local/udev/
   ```
4. Crear el lanzador de escritorio:
   ```
   mkdir -p ~/.local/share/applications
   cat > ~/.local/share/applications/tegrarcm.desktop <<'EOF'
   [Desktop Entry]
   Type=Application
   Name=TegraRcmGUI
   Comment=Inyector de payloads RCM para Nintendo Switch
   Exec=/home/%u/.local/bin/tegrarcm-gui
   Icon=tegrarcm
   Categories=Utility;System;
   Terminal=false
   EOF
   ```
   Reemplazá `Exec=` por la ruta absoluta real a tu `~/.local/bin/tegrarcm-gui` (los `.desktop` no expanden
   `~`). El ícono ya está embebido en el binario como recurso Qt (`assets/tegrarcm.svg` vía
   `assets/resources.qrc`), así que no hace falta instalar un `.svg`/`.png` aparte para que la propia
   ventana y el ícono de bandeja lo muestren; `Icon=tegrarcm` en el `.desktop` es solo para que el
   launcher del escritorio (menú de aplicaciones, dock) lo reconozca — si querés que también aparezca ahí,
   copiá el SVG a `~/.local/share/icons/hicolor/scalable/apps/tegrarcm.svg`.

### Alternativa: `cmake --install`

El `CMakeLists.txt` incluye un target de instalación simple que respeta el mismo layout relativo
(`bin/`, `tools/`, `udev/` como hermanos bajo el prefix):

```
cmake --install build --prefix ~/.local
```

Esto copia `tegrarcm-gui` a `~/.local/bin/`, `launcher_rcm` + `intermezzo.bin` a `~/.local/tools/`, la regla udev a
`~/.local/udev/` y este README a `~/.local/share/doc/tegrarcm/`. Con este método el paso 2 y 3 de arriba
quedan cubiertos automáticamente; solo faltan el `.desktop` (paso 4) y el udev (ver debajo).

### Estructura de rutas (por qué importa)

El binario busca sus archivos externos en dos ubicaciones relativas a sí mismo, en este orden:
`../tools/launcher_rcm` (o `../udev/50-tegrarcm.rules`) y luego `./tools/...` (o `./udev/...`). Esto
funciona tal cual en el árbol de build (`build/tegrarcm-gui` + `tools/` y `udev/` en la raíz del repo) y
tal cual con el layout `~/.local/{bin,tools,udev}/` de arriba. Si copiás el binario a otro lado sin
respetar esta estructura, la inyección y la instalación de udev van a fallar con "no encontrado".

## Reglas udev (por qué no hace falta root)

RCM expone un dispositivo USB "APX" (VID `0955`, PID `7321`) que por defecto solo root puede abrir. La
regla udev le da permisos `0666` a ese device específico para que un usuario normal pueda inyectar sin
`sudo` en cada uso — root solo hace falta una vez, al instalar la regla.

**Opción A — desde la app:** pestaña *Settings* → "Instalar reglas udev". Pide autenticación gráfica vía
`pkexec` (o `sudo` si `pkexec` no está disponible) y hace todo el proceso (copiar + recargar + trigger).

**Opción B — manual:**
```
sudo cp udev/50-tegrarcm.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules
sudo udevadm trigger
```

Desconectá y reconectá el cable USB (o volvé a entrar a RCM) después de instalar la regla para que se
aplique al dispositivo.

## Uso

1. Poné la Switch en modo RCM (unpatched): con la consola apagada, mantené VOL+ y presioná HOME mientras
   insertás el cable USB / jig de RCM, según tu método habitual.
2. Abrí la app. La barra de estado muestra "Waiting for device..." hasta que RCM aparece.
3. Pestaña *Payloads* → "Select payload" → elegí un `.bin` (por ejemplo, algo en `payloads/`).
4. "Inject". El estado pasa a "Injecting..." y después a éxito o error.

También podés guardar payloads como favoritos (pestaña *Favoritos*, doble click para inyectar) y usar la
pestaña *Tools* para ShofEL2, memloader o biskeydump.

## Payloads

La carpeta `payloads/` del repo es un lugar cómodo para guardar los `.bin` que uses seguido (no está
versionada por git salvo que la agregues vos a mano). Payloads típicos: [hekate](https://github.com/CTCaer/hekate)
(bootloader CFW), fusee de [Atmosphère](https://github.com/Atmosphere-NX/Atmosphere), ShofEL2, memloader,
biskeydump — todos enlazados desde el proyecto original.

La app valida el tamaño del archivo antes de inyectar: tiene que estar entre 4096 bytes (`0x1000`) y 1 MiB.
Fuera de ese rango, el botón "Inject" falla con un mensaje claro en vez de intentar enviarlo.

## Troubleshooting / FAQ

**"Device not detected" / se queda en "Waiting for device..."**
- Verificá que instalaste la regla udev (sección de arriba) y que reconectaste el cable después.
- Probá otro cable USB — muchos cables de solo carga no tienen líneas de datos.
- Confirmá que tu unidad es unpatched (https://ismyswitchpatched.com/): en unidades patcheadas, RCM
  arranca pero el exploit no corre y el device puede desconectarse solo.
- `lsusb` debería listar `0955:7321` mientras la consola está en RCM. Si no aparece ahí, es un problema de
  cable/hardware, no de la app.

**"Injection fails" con el device detectado**
- Revisá el tamaño del payload (sección de arriba); payloads truncados o corruptos fallan la validación.
- El motor (`launcher_rcm`) es un binario C que recibe el device path, el payload y el intermezzo como
  argumentos; el disparo usa el trigger oficial (GET_STATUS con recipient ENDPOINT, URB submit-only), que
  enviar) — si el error persiste, mirá el log de la pestaña correspondiente o `~/.local/share/tegrarcm/log.txt`
  para el detalle exacto que devolvió Python/pyusb.
- Confirmá que `python3` y el módulo `usb` (paquete `python-pyusb`) están instalados y accesibles en el
  PATH que ve la app.

**"pkexec hangs" / no aparece el diálogo de autenticación**
- `pkexec` necesita un agente de polkit corriendo en una sesión gráfica (GNOME, KDE, XFCE con
  `polkit-gnome`/`polkit-kde-agent` lo traen por defecto). Si corrés la app desde una TTY sin sesión
  gráfica, o vía SSH sin `DISPLAY`, `pkexec` se queda esperando sin mostrar nada.
- Solución: corré la app desde tu sesión de escritorio normal, o instalá las reglas manualmente
  (Opción B de la sección udev) con `sudo` desde una terminal.

## Licencia

GPL-2.0, igual que el proyecto original ([eliboa/TegraRcmGUI](https://github.com/eliboa/TegraRcmGUI)) y
[fusee-launcher](https://github.com/Cease-and-DeSwitch/fusee-launcher) de Kate Temkin / fail0verflow, del
que `tools/fusee-launcher.py` es una copia. Ver [`LICENSE`](LICENSE).

## Créditos

- [eliboa](https://github.com/eliboa) — TegraRcmGUI original (Windows/MFC)
- [Rajkosto](https://github.com/rajkosto) — TegraRcmSmash, memloader, biskeydump
- [Kate Temkin](https://github.com/ktemkin) / fail0verflow — Fusée Launcher, exploit Fusée Gelée
- [CTCaer](https://github.com/CTCaer/hekate) — Hekate
- [SciresM](https://github.com/SciresM) — Atmosphère
