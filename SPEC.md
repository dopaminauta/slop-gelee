# slop-gelee — linux port spec (v2, improved)

**Date:** 23/08/2026
**spec author:** dopaminauta + camarón (planning/architecture) → implementation: claude code
**Repo:** ~/Projects/tegrarcm-linux (clone of eliboa/TegraRcmGUI, GPL-2.0)
**Status:** Windows port (MFC/Win32) → Linux (Qt6/libusb)

> **Framing note:** this project is a platform port of an open-source tool (GPL-2.0) for loading recovery firmware on your own consoles (homebrew). It's the equivalent of porting a BIOS flasher or a recovery loader. All the code is public and the purpose is free-software interoperability.

---

## 0. implementer role (read first)

You are a **senior embedded C++/Qt engineer** porting an existing tool. Your job is NOT to redesign: it's to **transplant features and logic** from one platform to another with the least possible change, keeping the original's semantics. Every design decision must be justifiable with "that's how the original did it" or "on Linux this is done this way because X". If you find a decision you can't justify, document it as a DEVIATION in the final report — don't resolve it by making things up.

Golden rule: **the original is the behavior specification.** When the spec and the original differ, the original wins (unless the spec explicitly says otherwise).

---

## 1. objective

Port TegraRcmGUI (C++ GUI for TegraRcmSmash, Fusée Gelée payload injector for Nintendo Switch) to Linux, keeping ALL of the original's features. It must compile on CachyOS (Arch-based) with the standard toolchain, run without root (via udev rules), and be maintainable.

## 2. domain glossary (so you don't hallucinate the domain)

| Term | Meaning |
|---|---|
| **RCM** | Switch Recovery Mode. A NVIDIA Tegra boot mode that exposes an "APX" USB device (VID 0x0955, PID 0x7321) — the hardware's standard service/recovery mode |
| **Fusée Gelée** | Publicly documented boot sequence (fail0verflow, 2018) that allows loading recovery firmware on the Switch. Only works on "unpatched" units (manufactured before July 2018) |
| **Payload** | .bin file loaded into the Switch in recovery mode. E.g.: hekate (CFW bootloader), fusee.bin (Atmosphere), ShofEL2 (Linux), memloader, biskeydump |
| **TegraRcmSmash** | Reimplementation of the recovery payload loader by rajkosto. It's the ENGINE that does the loading. The repo has it embedded in TegraRcmGUI/TegraRcmSmash.cpp |
| **libusbk** | Windows USB library used by the original. Does NOT exist on Linux → replaced by libusb-1.0 |
| **ShofEL2** | fail0verflow's boot stack to run Linux on the Switch. It "runs" by loading its payload |
| **memloader** | rajkosto's payload that turns the Switch into a USB mass storage device (SD/eMMC show up as /dev/sdX on Linux) |
| **BIS keys** | eMMC encryption keys. biskeydump is the payload that extracts them |
| **APX driver** | On Windows you must install a driver. On Linux there's NO driver: solved with udev rules (permissions) |

## 3. architecture decisions (already made — don't question them)

| Layer | Original (Windows) | Port (Linux) |
|---|---|---|
| UI | MFC dialogs + Win32 | **Qt6 Widgets** (QMainWindow + QTabWidget) |
| USB backend | libusbk | **libusb-1.0** |
| Injection engine | TegraRcmSmash.cpp (embedded) | **Keep it**, adapt the libusbk→libusb transport layer |
| Build | MSVC .sln/.vcxproj | **CMake ≥ 3.25** |
| Tray | Win32 Shell_NotifyIcon | **QSystemTrayIcon** |
| Startup | Windows Registry | **XDG autostart** (~/.config/autostart/*.desktop) |
| APX driver | Zadig/libusbk | **udev rules** (VID 0955 PID 7321, MODE="0666") |
| File dialogs | Win32 GetOpenFileName | **QFileDialog** |
| Persistence | Registry/ini files | **~/.config/tegrarcm/** (JSON or QSettings) |

## 4. original features (all — MoSCoW prioritization by phase)

| Feature | Priority | Phase |
|---|---|---|
| 1. Load payloads (recovery firmware) | **Must** | 1 |
| 2. Auto-load (when selecting and/or when connecting) | **Must** | 1 (connect) / 2 (select) |
| 3. Hotplug detection (status in UI) | **Must** | 1 |
| 4. Manage favorites (persistent) | **Must** | 2 |
| 5. Minimize to tray + context menu | **Should** | 2 |
| 6. Run at startup (XDG autostart) | **Should** | 3 |
| 7. Install udev rules (with sudo) | **Must** | 3 |
| 8. Run Linux (ShofEL2) | **Should** | 3 |
| 9. Mount USB mass storage (memloader) | **Could** | 3 |
| 10. Dump BIS keys (biskeydump) | **Could** | 3 |

## 5. technical protocol — libusbk → libusb-1.0 mapping (critical, don't improvise)

### 5.1 the mechanism (context — the engine already implements it, don't rewrite it)
The Switch in RCM (VID 0x0955, PID 0x7321) exposes a known bug in the GET_DESCRIPTOR response. The payload is written with a vendor control transfer (bRequest 0x80) to address 0x40010000 (SMEM), followed by the size handshake. The code is in `TegraRcmGUI/TegraRcmSmash.cpp` — copy it to `src/SmashEngine.cpp` and adapt ONLY the transport layer.

### 5.2 call mapping (mandatory reference table)

| libusbk (original) | libusb-1.0 (port) |
|---|---|
| `USBSmashContext` / `Context->Init()` | `libusb_init(&ctx)` + `libusb_set_option(ctx, LIBUSB_OPTION_LOG_LEVEL, ...)` |
| `Context->GetDevice(...)` | `libusb_get_device_list(ctx, &list)` + VID/PID filter, or `libusb_open_device_with_vid_pid` |
| `Device->Open(VID, PID)` | `libusb_open(dev, &handle)` |
| `Device->ClaimInterface(0)` | `libusb_claim_interface(handle, 0)` |
| `Device->ControlTransfer(...)` | `libusb_control_transfer(handle, bmRequestType, bRequest, wValue, wIndex, data, wLength, timeout)` |
| `Device->BulkTransfer(...)` | `libusb_bulk_transfer(handle, endpoint, data, length, &transferred, timeout)` |
| `Device->Close()` / Context->Exit() | `libusb_close(handle)` / `libusb_exit(ctx)` |
| `USBSmash::Smash()` (logic) | **KEEP** — only change the transport calls |

**Gotcha #1 (kernel driver):** before claim_interface, call `libusb_set_auto_detach_kernel_driver(handle, 1)` so libusb automatically detaches kernel drivers (avoids "Device or resource busy" errors on Linux).

**Gotcha #2 (hotplug + Qt):** `libusb_hotplug_register_callback` requires `libusb_handle_events()` to run continuously. **DON'T run it on Qt's main thread** (it would block the event loop). Correct pattern:
- Create a `QThread` worker (`InjectorWorker`) that runs `libusb_handle_events_completed` in a loop with a timeout (e.g.: 100ms, checking a stop flag).
- Emit Qt signals (`deviceConnected()`, `deviceDisconnected()`) from the worker → the UI receives them via queued connections.
- Acceptable alternative: a 250ms QTimer that calls `libusb_get_device_list` and compares (polling). Simpler, less elegant. Choose ONE and justify it in the report.

**Gotcha #3 (injection endpoint):** the injection control transfer uses bmRequestType `0x21` (vendor, host→device, interface), bRequest `0x80`, wValue `0x1000`. It's in the original engine — don't change it.

### 5.3 device detection
- VID 0x0955, PID 0x7321 (NVIDIA APX in RCM)
- Note: when the Switch is NOT in RCM (normal boot), this device doesn't show up. The "waiting for device" status is the default state.

## 6. file structure (create at the root, next to the original)

```
tegrarcm-linux/
├── CMakeLists.txt                 # new build (Linux) — Phase 1
├── src/
│   ├── main.cpp                   # Qt6 entry point — Phase 1
│   ├── MainWindow.h/.cpp          # main window + tabs + status bar — Phase 1
│   ├── PayloadTab.h/.cpp          # tab 1: selection + injection — Phase 1
│   ├── FavoritesTab.h/.cpp        # tab 2: favorites — Phase 2
│   ├── ToolsTab.h/.cpp            # tab 3: ShofEL2, memloader, biskeydump — Phase 3
│   ├── SettingsTab.h/.cpp         # tab 4: auto-inject, tray, startup, udev — Phase 3
│   ├── Injector.h/.cpp            # libusb wrapper + hotplug worker (QThread) — Phase 1
│   ├── SmashEngine.h/.cpp         # adapted TegraRcmSmash.cpp — Phase 1
│   ├── Favorites.h/.cpp           # JSON persistence — Phase 2
│   └── UdevRules.h/.cpp           # udev rules installation — Phase 3
├── udev/
│   └── 50-tegrarcm.rules          # SUBSYSTEM=="usb", ATTR{idVendor}=="0955", ATTR{idProduct}=="7321", MODE="0666"
├── payloads/                      # folder for the user's payloads (gitignored)
└── README.md                      # build + usage + udev — Phase 4
```

## 7. ui/logic contract (separation — mandatory)

The `Injector` must NOT include Qt Widgets headers. Use signals/slots:

```cpp
// Injector.h (sketch — not the final file, it's the contract)
class Injector : public QObject {
    Q_OBJECT
public:
    explicit Injector(QObject* parent = nullptr);
    ~Injector();

    // State
    bool isDeviceConnected() const;

    // Actions (all async or with timeout — never block the UI)
    void injectPayload(const QString& payloadPath);   // emits payloadInjected / injectionFailed
    void startMonitoring();                            // starts the hotplug worker
    void stopMonitoring();

signals:
    void deviceConnected();
    void deviceDisconnected();
    void payloadInjected(const QString& payloadPath, int bytesSent);
    void injectionFailed(const QString& errorMessage);
    void logMessage(const QString& message);           // for the UI log

private:
    class Worker;  // QThread internals
    // ... libusb ctx, handle, hotplug handle, worker thread
};
```

The UI (tabs) connects to these signals. The hotplug worker is an internal detail of `Injector`.

## 8. detailed functional requirements

### 8.1 payloads tab (phase 1) — must
- [ ] "Select payload" button → QFileDialog, `*.bin` filter, remembers the last directory (QSettings)
- [ ] QLineEdit (readonly) with the payload path + "Inject" button
- [ ] Status in the status bar: "Waiting for device..." / "Device found (RCM)" / "Injecting..." / "Payload injected (N bytes)"
- [ ] "Auto inject when selected" checkbox (injects when choosing a payload) — Phase 2 wire-up, UI already in Phase 1
- [ ] "Save as favorite" button (disabled until Phase 2, or functional if Favorites already exists)
- [ ] Loading states: "Loading..." (disable buttons), success/error (re-enable + message)
- [ ] Validation: if there's no device, "Load" shows a clear error "No RCM device detected. Put your Switch in RCM mode."

### 8.2 favorites tab (phase 2) — must
- [ ] QListWidget of saved payloads
- [ ] Double click → inject directly
- [ ] Right click → menu: "Inject", "Remove"
- [ ] Persistence: ~/.config/tegrarcm/favorites.json (format: `{"favorites": ["/path/a.bin", ...]}`)
- [ ] Injecting from favorites: same status flow as tab 1

### 8.3 tools tab (phase 3) — should
- [ ] "Run Linux (ShofEL2)" button → injects the selected Linux payload (reuses the injection mechanism)
- [ ] "Mount eMMC/SD (memloader)" button → injects memloader v3; after injecting, detects the new block device (compare /dev/sdX before/after, or udevadm monitor) and shows the path + mount instructions
- [ ] "Dump BIS keys" button → injects biskeydump
- [ ] QPlainTextEdit output log (appends each operation with a timestamp)
- [ ] Each tool needs its .bin payload: look in ~/.config/tegrarcm/payloads/ or ask the user the first time (QFileDialog) and remember it

### 8.4 settings tab (phase 3) — should
- [ ] "Auto-inject when device connected" checkbox (persistent)
- [ ] "Minimize to tray" checkbox (persistent)
- [ ] "Run at startup" checkbox → creates/deletes ~/.config/autostart/tegrarcm.desktop
- [ ] "Install udev rules" button → `pkexec` or `sudo` copying udev/50-tegrarcm.rules to /etc/udev/rules.d/ + `udevadm control --reload-rules` + `udevadm trigger`; shows the result
- [ ] "Open payloads folder" button → opens the payloads folder with QDesktopServices

## 9. non-functional requirements (mandatory)

- **No root for normal use:** udev rules MODE="0666" → regular user can inject
- **Clean code:** UI/logic separation (section 7), descriptive names, comments ONLY where they explain "why" (not "what")
- **Logging:** qDebug + file ~/.local/share/tegrarcm/log.txt (simple rotation: if > 1MB, rename to .old)
- **Build:** CMake `find_package(Qt6 COMPONENTS Widgets REQUIRED)` + `pkg_check_modules(LIBUSB REQUIRED IMPORTED_TARGET libusb-1.0)`
- **C++17**, `-Wall -Wextra` without warnings
- **Minimal i18n:** strings in Spanish (the user is Argentine); structure ready for future translation (Qt tr())
- **Error handling:** every libusb call checks its return code; errors → logMessage signal + status bar
- **No crashes:** without a device, with the device removed mid-injection, with an invalid payload (check size ≥ 0x1000 and ≤ 1MB), with a nonexistent file

## 10. phased implementation plan

### Phase 1 — MVP (in progress)
- [ ] Complete CMakeLists.txt + src/ structure
- [ ] SmashEngine: TegraRcmSmash.cpp adapted to libusb (section 5.2 mapping)
- [ ] MainWindow: 4 tabs (real Payloads, other placeholders) + status bar
- [ ] Payloads tab: select + inject + status + validations
- [ ] Hotplug: QThread worker (gotcha #2) → deviceConnected/Disconnected → status bar
- [ ] Build: `cmake -B build && cmake --build build -j$(nproc)` → `tegrarcm-gui` binary
- [ ] Manual test without hardware: opens, doesn't crash, shows "Waiting for device"
- [ ] Hardware test (when Axel connects the Switch): device found + real injection

### Phase 2 — favorites + auto-inject + tray
- [ ] Favorites.h/.cpp + complete Favorites tab (8.2)
- [ ] Auto-inject when the device connects (persistent setting)
- [ ] Tray icon (QSystemTrayIcon): show/hide, "Inject last payload", "Quit"
- [ ] Minimize to tray (intercept closeEvent → hide if enabled)

### Phase 3 — tools + settings + udev
- [ ] Complete Tools tab (8.3)
- [ ] Complete Settings tab (8.4) + persistence (QSettings: ~/.config/tegrarcm/tegrarcm.conf)
- [ ] UdevRules: udev/50-tegrarcm.rules + installation with pkexec/sudo
- [ ] File logging with rotation

### Phase 4 — polish + packaging
- [ ] Complete README: build, udev install, usage, troubleshooting (FAQ: "device not detected" → check udev, cable, patched unit)
- [ ] App icon (assets/icon.png, embedded in the binary via QResource or relative path)
- [ ] Full test with real hardware (Axel's unpatched Switch)
- [ ] Optional: CPack/AppImage

## 11. acceptance criteria (per phase — verifiable by command)

### Phase 1
- [ ] `cmake -B build && cmake --build build -j$(nproc)` → exit 0, no warnings
- [ ] `./build/tegrarcm-gui` opens, shows 4 tabs, status bar with "Waiting for device..."
- [ ] With the Switch in RCM: status changes to "Device found (RCM)" in < 1s
- [ ] Select payload + Inject → the Switch boots (hardware test)
- [ ] Without the Switch: the app doesn't crash, injecting shows a clear error
- [ ] Closing the app doesn't leave threads hanging (clean exit)

### Phase 2
- [ ] Favorites persist across sessions (create, restart the app, they're still there)
- [ ] Auto-inject works when connecting the Switch
- [ ] Tray: minimize, restore, quit

### Phase 3
- [ ] udev install: runs with pkexec, rule present, udevadm reload, injection works without root
- [ ] ShofEL2/memloader/biskeydump: inject the correct payload (verified by log + Switch behavior)
- [ ] Startup toggle creates/deletes the .desktop

## 12. anti-patterns and prohibitions (violating these = rework)

1. **DON'T modify files in TegraRcmGUI/** (the original) — read-only for reference
2. **DON'T rewrite the SmashEngine** engine from scratch — the protocol is already implemented, only the transport is adapted
3. **DON'T invent APIs** — verify against installed headers (pkg-config --cflags libusb-1.0, /usr/include/qt6/)
4. **DON'T block the main thread** with libusb_handle_events or long USB operations
5. **DON'T hardcode user paths** — use QStandardPaths / QDir::homePath
6. **DON'T leave raw memory without RAII** — use QScopedPointer/unique_ptr for libusb ctx/handle
7. **DON'T use system() to copy udev** — use QProcess with pkexec/sudo (the graphical auth prompt is part of the UX)
8. **DON'T ignore libusb return codes** — each one is checked, errors propagate as signals
9. **DON'T assume the device appears instantly** — hotplug/polling with retry
10. **DON'T comment obvious code** — comment the "why", never the "what"

## 13. final report format (mandatory at the end of each phase)

```
## implementation report — Phase N

### what was built
(files created/modified, one line of purpose each)

### changes over the original
(when applicable: what was adapted in SmashEngine and why — table if possible)

### design decisions made
(each with its justification. If a decision differs from the spec, mark it as deviation and explain)

### build and verification
(real cmake --build output, results of the manual tests you could run)

### pending / risks
(what couldn't be tested without hardware, what might fail in integration)

### suggested next step
(one line — which phase comes next and with what focus)
```

## 14. restrictions for Claude Code (executable summary)

- Work ONLY in: src/, CMakeLists.txt, udev/, README.md, assets/
- Read (never modify): TegraRcmGUI/ (original)
- Copy TegraRcmSmash.cpp → src/SmashEngine.cpp and adapt; document changes in a comment at the top of the file
- Keep GPL-2.0 headers in derived files
- Each phase ends with: build OK + report (section 13)
- If something in the spec is ambiguous: decide, document it as DEVIATION, move on — DON'T ask (autonomous mode)
