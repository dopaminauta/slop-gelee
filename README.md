![GitHub](https://img.shields.io/github/license/eliboa/TegraRcmGUI)

# slop-gelee

a linux port of [TegraRcmGUI](https://github.com/eliboa/TegraRcmGUI) (gpl-2.0). loads fusée gelée payloads onto a nintendo switch (t210 erista) over usb in rcm mode. homebrew on your own hardware.

the name is a joke. the code is not.

## why this exists

the original is windows-only. this one runs on linux. that's mostly it. qt6 for the gui, libusb for the usb, and a plain C engine (`launcher_rcm`) for the injection.

about that engine: it uses the official trigger, the get_status with endpoint recipient, submitted without the discard that cancels the urb before it travels. we spent two hours on that because we copied the nxloader replica first and it looked identical in the logs. the report that explains it all is [here](https://github.com/ktemkin/fusee-launcher/blob/master/report/fusee_gelee.md). read it before you touch any of this.

## dependencies

```
sudo pacman -S qt6-base qt6-svg libusb
```

plus cmake and a c++17 compiler.

## build

```
cmake -B build && cmake --build build -j$(nproc)
```

the binary lands in `build/`. it looks for `tools/launcher_rcm`, `tools/intermezzo.bin` and `udev/50-tegrarcm.rules` relative to itself, so keep the layout.

## install

the release tarball has an `install.sh` that does the whole thing:

```
tar -xzf slop-gelee-v1.0.0-linux-x86_64.tar.gz
cd slop-gelee-v1.0.0-linux-x86_64
./install.sh
```

add `--with-udev` if you want the udev rules active. that's the only step that needs sudo, and only once.

## usage

1. put the switch in rcm mode (vulnerable unit, jig + vol+ + power).
2. open the app. the status bar says "waiting for device..." until rcm shows up.
3. main tab: pick a payload, hit inject.
4. if it's a fresh rcm session it boots. if you waited too long it won't, the rcm mode lasts about a minute. there's an auto-inject toggle in settings for that.

favorites live on the main tab too, double-click to inject. the tools tab has shofel2 to run linux, biskeydump and lockpick_rcm for keys, each with its default payload shipped in the release.

## things that will bite you

- only t210 erista units work. hardware-patched ones boot rcm but the exploit does nothing. check https://ismyswitchpatched.com/
- rcm answers the device id once per usb session. if something else reads it first (a diagnostic script, a curious python one-liner), the injection will fail with a read timeout and you have to re-enter rcm. we learned that the hard way.
- charge-only usb cables exist and they will waste your evening.
- the payload must be between 4096 bytes and 1 mb, the app checks.

## license

gpl-2.0, same as the original. see [`LICENSE`](LICENSE).

## credits

- [eliboa](https://github.com/eliboa) — tegrarcmgui
- [Kate Temkin](https://github.com/ktemkin) / fail0verflow — fusée gelée
- [CTCaer](https://github.com/CTCaer/hekate) — hekate
- [SciresM](https://github.com/SciresM) — atmosphère
