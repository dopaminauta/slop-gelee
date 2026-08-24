![GitHub](https://img.shields.io/github/license/eliboa/TegraRcmGUI)

# slop-gelee

a Linux port (Qt6 + libusb) of [TegraRcmGUI](https://github.com/eliboa/TegraRcmGUI), a C++ GUI for
loading recovery *payloads* (Fusée Gelée) onto a Nintendo Switch over USB in RCM (Recovery Mode).
the original is Windows-only (MFC/Win32 + libusbk); this port replaces that layer with Qt6 Widgets
and libusb-1.0, keeping the rest of the features.

> framing note: this is a platform port of an open-source (GPL-2.0) tool for homebrew interoperability
> with your own hardware. the equivalent would be porting a BIOS flasher or a recovery loader. it only
> works on Switch units vulnerable to Fusée Gelée (Erista T210; hardware-unpatched, or firmware-patched
> which work fine) — check https://ismyswitchpatched.com/

the name is a joke: built with a paperclip (the jig), willpower, and a suspicious amount of AI slop.
the code is GPL-2.0 and the trigger is the official one.

## dependencies (arch / cachyos)

```
sudo pacman -S qt6-base qt6-svg libusb
```

- **qt6-base** — Qt6 Widgets (UI)
- **qt6-svg** — Qt SVG engine, needed for the app icon
- **libusb** — USB layer (RCM device detection)
- **launcher_rcm** — native C injection engine (included in the repo and in releases), a replica of the
  NXLoader engine corrected against ktemkin's official PoC (ENDPOINT recipient + submit-only URB)
- **pkexec** (part of `polkit`, usually present on a desktop) — used by the "Install udev rules" button
  in Settings for graphical authentication

CMake, a C++17 compiler and `pkg-config` are needed to build (`base-devel`, `cmake`).

## build

```
cmake -B build && cmake --build build -j$(nproc)
```

this produces `build/tegrarcm-gui`. you can run it straight from there — it looks for
`tools/launcher_rcm`, `tools/intermezzo.bin` and `udev/50-tegrarcm.rules` relative to the repo root
(see "path layout" below), so no install is needed to try it from the build tree.

## install

1. copy the binary:
   ```
   mkdir -p ~/.local/bin
   cp build/tegrarcm-gui ~/.local/bin/
   ```
2. copy the injection engine **next to** `bin/`, not inside (the app looks for `../tools/launcher_rcm`
   and `../tools/intermezzo.bin` relative to the binary):
   ```
   mkdir -p ~/.local/tools
   cp tools/launcher_rcm tools/intermezzo.bin ~/.local/tools/
   ```
3. (optional) copy the udev rules the same way, so the "Install udev rules" button finds them without
   pointing at the repo:
   ```
   mkdir -p ~/.local/udev
   cp udev/50-tegrarcm.rules ~/.local/udev/
   ```
4. create a desktop launcher:
   ```
   mkdir -p ~/.local/share/applications
   cat > ~/.local/share/applications/tegrarcm.desktop <<'EOF'
   [Desktop Entry]
   Type=Application
   Name=slop gelee
   Comment=RCM payload injector for Nintendo Switch
   Exec=/home/%u/.local/bin/tegrarcm-gui
   Icon=tegrarcm
   Categories=Utility;System;
   Terminal=false
   EOF
   ```
   replace `Exec=` with the real absolute path to your `~/.local/bin/tegrarcm-gui` (`.desktop` files do
   not expand `~`). the icon is embedded in the binary as a Qt resource (`assets/tegrarcm.svg` via
   `assets/resources.qrc`), so no separate install is needed for the window/tray icon; `Icon=tegrarcm`
   in the `.desktop` is only for the desktop launcher (app menu, dock) — copy the SVG to
   `~/.local/share/icons/hicolor/scalable/apps/tegrarcm.svg` if you want it there too.

   easier: the release tarball ships an `install.sh` that does all of the above (and the desktop entry
   with icon), optionally installing udev rules with `./install.sh --with-udev`.

### alternative: `cmake --install`

the `CMakeLists.txt` has a simple install target that keeps the same relative layout (`bin/`, `tools/`,
`udev/` as siblings under the prefix):

```
cmake --install build --prefix ~/.local
```

this copies `tegrarcm-gui` to `~/.local/bin/`, `launcher_rcm` + `intermezzo.bin` to `~/.local/tools/`,
the udev rule to `~/.local/udev/` and this README to `~/.local/share/doc/tegrarcm/`. steps 2 and 3
above are covered automatically; you still need the `.desktop` (step 4) and the udev activation.

### path layout (why it matters)

the binary looks for its external files in two locations relative to itself, in this order:
`../tools/launcher_rcm` (or `../udev/50-tegrarcm.rules`) and then `./tools/...` (or `./udev/...`). this
works as-is in the build tree (`build/tegrarcm-gui` + `tools/` and `udev/` at the repo root) and with
the `~/.local/{bin,tools,udev}/` layout above. if you copy the binary somewhere else without keeping
this structure, injection and udev installation will fail with "not found".

## udev rules (why root is not needed)

RCM exposes an "APX" USB device (VID `0955`, PID `7321`) that only root can open by default. the udev
rule gives `0666` permissions to that specific device so a normal user can inject without `sudo` every
time — root is only needed once, to install the rule.

**option A — from the app:** Settings → "Install udev rules". asks for graphical authentication via
`pkexec` (or `sudo` if `pkexec` is unavailable) and does everything (copy + reload + trigger).

**option B — manual:**
```
sudo cp udev/50-tegrarcm.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules
sudo udevadm trigger
```

unplug and replug the USB cable (or re-enter RCM) after installing the rule so it applies to the device.

## usage

1. put the Switch in RCM mode (vulnerable unit): with the console off, hold VOL+ and press power while
   the jig shorts the right joycon pin, per your usual method.
2. open the app. the status bar shows "Waiting for device..." until RCM appears.
3. "main" tab → "Select payload..." → pick a `.bin` (e.g. something in `payloads/`).
4. "Inject". status goes to "Injecting..." and then success or error.

you can also save payloads as favorites (double-click in the favorites list on the main tab) and use the
Tools tab for ShofEL2, biskeydump or Lockpick_RCM — each with its default payload included in releases.

> tip: RCM mode has a short window (~1 min). connect the console and inject fast, or enable
> "Auto-inject when device connects" in Settings and just plug it in.

## payloads

the `payloads/` folder in the repo is a handy place for `.bin` files you use often. typical payloads:
[hekate](https://github.com/CTCaer/hekate) (CFW bootloader), fusee from
[Atmosphère](https://github.com/Atmosphere-NX/Atmosphere), ShofEL2, biskeydump, Lockpick_RCM.

the app validates file size before injecting: between 4096 bytes (`0x1000`) and 1 MiB. outside that
range, "Inject" fails with a clear message instead of trying to send it.

## troubleshooting / faq

**"device not detected" / stuck on "Waiting for device..."**
- make sure you installed the udev rule (section above) and replugged the cable afterwards.
- try another USB cable — many charge-only cables have no data lines.
- confirm your unit is vulnerable (https://ismyswitchpatched.com/): on hardware-patched units RCM
  boots but the exploit does not run.
- `lsusb` should list `0955:7321` while the console is in RCM. if it does not, it's a cable/hardware
  problem, not the app.

**"injection failed" with the device detected**
- check the payload size (section above); truncated or corrupt payloads fail validation.
- the engine (`launcher_rcm`) is a C binary that takes the device path, payload and intermezzo as
  arguments; the trigger is the official one (GET_STATUS with ENDPOINT recipient, submit-only URB). if
  the error persists, look at the log in the Tools tab or `~/.local/share/tegrarcm/log.txt` for the
  exact detail.
- if a read of the device ID times out, the RCM session was likely already consumed (the ID is answered
  once per USB session) — re-enter RCM mode and inject without touching the device first.

**"pkexec hangs" / no auth dialog**
- `pkexec` needs a polkit agent running in a graphical session (GNOME, KDE, XFCE ship one by default).
  if you run the app from a TTY without a graphical session, or over SSH without `DISPLAY`, `pkexec`
  waits forever.
- fix: run the app from your normal desktop session, or install the rules manually (option B above)
  with `sudo` from a terminal.

## license

GPL-2.0, same as the original project ([eliboa/TegraRcmGUI](https://github.com/eliboa/TegraRcmGUI)) and
[fusee-launcher](https://github.com/ktemkin/fusee-launcher) by Kate Temkin / fail0verflow, which the
engine is based on. see [`LICENSE`](LICENSE).

## credits

- [eliboa](https://github.com/eliboa) — original TegraRcmGUI (Windows/MFC)
- [Rajkosto](https://github.com/rajkosto) — TegraRcmSmash, memloader, biskeydump
- [Kate Temkin](https://github.com/ktemkin) / fail0verflow — Fusée Launcher, Fusée Gelée exploit
- [CTCaer](https://github.com/CTCaer/hekate) — Hekate
- [SciresM](https://github.com/SciresM) — Atmosphère
