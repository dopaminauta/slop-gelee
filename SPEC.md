# TegraRcmGUI-Linux — SPEC TÉCNICO DE PORTEO (v2, mejorado)

**Fecha:** 23/08/2026
**Autor del spec:** Camarón (planificación/arquitectura) → implementación: Claude Code
**Repo:** ~/Projects/tegrarcm-linux (clone de eliboa/TegraRcmGUI, GPL-2.0)
**Estado:** Port de Windows (MFC/Win32) → Linux (Qt6/libusb)

> **Nota de encuadre:** este proyecto es un port de plataforma de una herramienta open-source (GPL-2.0) para carga de firmware de recuperación en consolas propias (homebrew). Es el equivalente a portar un flasher de BIOS o un loader de recovery. Todo el código es público y el propósito es interoperabilidad de software libre.

---

## 0. ROL DEL IMPLEMENTADOR (LEER PRIMERO)

Sos un **ingeniero senior de C++/Qt embebido** portando una herramienta existente. Tu trabajo NO es rediseñar: es **trasplantar features y lógica** de una plataforma a otra con el menor cambio posible, manteniendo la semántica del original. Cada decisión de diseño debe poder justificarse con "así lo hacía el original" o "en Linux esto se hace así porque X". Si encontrás una decisión que no podés justificar, documentala como DEVIACIÓN en el reporte final — no la resuelvas inventando.

Regla de oro: **el original es la especificación de comportamiento.** Cuando el spec y el original difieran, el original manda (salvo que el spec lo diga explícitamente).

---

## 1. OBJETIVO

Portar TegraRcmGUI (C++ GUI para TegraRcmSmash, inyector de payloads Fusée Gelée para Nintendo Switch) a Linux, conservando TODAS las features del original. Debe compilar en CachyOS (Arch-based) con toolchain estándar, correr sin root (vía udev rules), y ser mantenible.

## 2. GLOSARIO DE DOMINIO (para que NO alucines el dominio)

| Término | Significado |
|---|---|
| **RCM** | Recovery Mode del Switch. Es un modo de arranque NVIDIA Tegra que expone un dispositivo USB "APX" (VID 0x0955, PID 0x7321) — el modo de servicio/recovery estándar del hardware |
| **Fusée Gelée** | Secuencia de arranque documentada públicamente (fail0verflow, 2018) que permite cargar firmware de recuperación en Switch. Solo funciona en unidades "unpatched" (fabricadas antes de julio 2018) |
| **Payload** | Archivo .bin que se carga al Switch en modo recovery. Ej: hekate (bootloader CFW), fusee.bin (Atmosphere), ShofEL2 (Linux), memloader, biskeydump |
| **TegraRcmSmash** | Reimplementación del cargador de payloads de recovery por rajkosto. Es el MOTOR que hace la carga. El repo lo tiene embebido en TegraRcmGUI/TegraRcmSmash.cpp |
| **libusbk** | Librería USB de Windows que usa el original. NO existe en Linux → se reemplaza por libusb-1.0 |
| **ShofEL2** | Boot stack de fail0verflow para correr Linux en el Switch. Se "corre" cargando su payload |
| **memloader** | Payload de rajkosto que convierte al Switch en un dispositivo de almacenamiento USB masivo (SD/eMMC aparecen como /dev/sdX en Linux) |
| **BIS keys** | Claves de cifrado de eMMC. biskeydump es el payload que las extrae |
| **APX driver** | En Windows hay que instalar un driver. En Linux NO hay driver: se resuelve con udev rules (permisos) |

## 3. DECISIONES DE ARQUITECTURA (ya tomadas — no las cuestiones)

| Capa | Original (Windows) | Port (Linux) |
|---|---|---|
| UI | MFC dialogs + Win32 | **Qt6 Widgets** (QMainWindow + QTabWidget) |
| Backend USB | libusbk | **libusb-1.0** |
| Motor de inyección | TegraRcmSmash.cpp (embebido) | **Conservar**, adaptar capa de transporte libusbk→libusb |
| Build | MSVC .sln/.vcxproj | **CMake ≥ 3.25** |
| Tray | Win32 Shell_NotifyIcon | **QSystemTrayIcon** |
| Startup | Registro Windows | **XDG autostart** (~/.config/autostart/*.desktop) |
| Driver APX | Zadig/libusbk | **udev rules** (VID 0955 PID 7321, MODE="0666") |
| File dialogs | Win32 GetOpenFileName | **QFileDialog** |
| Persistencia | Registro/archivos ini | **~/.config/tegrarcm/** (JSON o QSettings) |

## 4. FEATURES DEL ORIGINAL (TODAS — priorización MoSCoW por fase)

| Feature | Prioridad | Fase |
|---|---|---|
| 1. Load payloads (firmware de recovery) | **Must** | 1 |
| 2. Auto-load (al seleccionar y/o al conectar) | **Must** | 1 (conectar) / 2 (seleccionar) |
| 3. Detección hotplug (status en UI) | **Must** | 1 |
| 4. Manage favorites (persistente) | **Must** | 2 |
| 5. Minimize to tray + menú contextual | **Should** | 2 |
| 6. Run at startup (XDG autostart) | **Should** | 3 |
| 7. Install udev rules (con sudo) | **Must** | 3 |
| 8. Run Linux (ShofEL2) | **Should** | 3 |
| 9. Mount USB mass storage (memloader) | **Could** | 3 |
| 10. Dump BIS keys (biskeydump) | **Could** | 3 |

## 5. PROTOCOLO TÉCNICO — MAPEO libusbk → libusb-1.0 (crítico, no improvises)

### 5.1 El mecanismo (contexto — el motor ya lo implementa, NO lo reescribas)
El Switch en RCM (VID 0x0955, PID 0x7321) expone un bug conocido en la respuesta a GET_DESCRIPTOR. El payload se escribe con un control transfer vendor (bRequest 0x80) al address 0x40010000 (SMEM), seguido del handshake del tamaño. El código está en `TegraRcmGUI/TegraRcmSmash.cpp` — copialo a `src/SmashEngine.cpp` y adaptá SOLO la capa de transporte.

### 5.2 Mapeo de llamadas (tabla de referencia obligatoria)

| libusbk (original) | libusb-1.0 (port) |
|---|---|
| `USBSmashContext` / `Context->Init()` | `libusb_init(&ctx)` + `libusb_set_option(ctx, LIBUSB_OPTION_LOG_LEVEL, ...)` |
| `Context->GetDevice(...)` | `libusb_get_device_list(ctx, &list)` + filtro VID/PID, o `libusb_open_device_with_vid_pid` |
| `Device->Open(VID, PID)` | `libusb_open(dev, &handle)` |
| `Device->ClaimInterface(0)` | `libusb_claim_interface(handle, 0)` |
| `Device->ControlTransfer(...)` | `libusb_control_transfer(handle, bmRequestType, bRequest, wValue, wIndex, data, wLength, timeout)` |
| `Device->BulkTransfer(...)` | `libusb_bulk_transfer(handle, endpoint, data, length, &transferred, timeout)` |
| `Device->Close()` / Context->Exit() | `libusb_close(handle)` / `libusb_exit(ctx)` |
| `USBSmash::Smash()` (lógica) | **CONSERVAR** — solo cambiar las llamadas de transporte |

**Gotcha #1 (kernel driver):** antes de claim_interface, llamar `libusb_set_auto_detach_kernel_driver(handle, 1)` para que libusb detache drivers del kernel automáticamente (evita errores "Device or resource busy" en Linux).

**Gotcha #2 (hotplug + Qt):** `libusb_hotplug_register_callback` requiere que `libusb_handle_events()` corra continuamente. **NO lo corras en el hilo principal de Qt** (bloquearía el event loop). Patrón correcto:
- Crear un `QThread` worker (`InjectorWorker`) que corre `libusb_handle_events_completed` en un loop con timeout (ej: 100ms, chequeando un flag de stop).
- Emitir signals Qt (`deviceConnected()`, `deviceDisconnected()`) desde el worker → el UI las recibe via queued connections.
- Alternativa aceptable: QTimer de 250ms que hace `libusb_get_device_list` y compara (polling). Más simple, menos elegante. Elegí UNA y justificala en el reporte.

**Gotcha #3 (endpoint de inyección):** el control transfer de inyección usa bmRequestType `0x21` (vendor, host→device, interface), bRequest `0x80`, wValue `0x1000`. Está en el motor original — no lo cambies.

### 5.3 Detección de dispositivo
- VID 0x0955, PID 0x7321 (NVIDIA APX en RCM)
- Nota: cuando el Switch NO está en RCM (encendido normal), NO aparece este device. El status "waiting for device" es el estado default.

## 6. ESTRUCTURA DE ARCHIVOS (crear en raíz, al lado del original)

```
tegrarcm-linux/
├── CMakeLists.txt                 # build nuevo (Linux) — Fase 1
├── src/
│   ├── main.cpp                   # entry point Qt6 — Fase 1
│   ├── MainWindow.h/.cpp          # ventana principal + tabs + status bar — Fase 1
│   ├── PayloadTab.h/.cpp          # tab 1: selección + inyección — Fase 1
│   ├── FavoritesTab.h/.cpp        # tab 2: favoritos — Fase 2
│   ├── ToolsTab.h/.cpp            # tab 3: ShofEL2, memloader, biskeydump — Fase 3
│   ├── SettingsTab.h/.cpp         # tab 4: auto-inject, tray, startup, udev — Fase 3
│   ├── Injector.h/.cpp            # wrapper libusb + hotplug worker (QThread) — Fase 1
│   ├── SmashEngine.h/.cpp         # TegraRcmSmash.cpp adaptado — Fase 1
│   ├── Favorites.h/.cpp           # persistencia JSON — Fase 2
│   └── UdevRules.h/.cpp           # instalación de reglas udev — Fase 3
├── udev/
│   └── 50-tegrarcm.rules          # SUBSYSTEM=="usb", ATTR{idVendor}=="0955", ATTR{idProduct}=="7321", MODE="0666"
├── payloads/                      # carpeta para payloads del usuario (gitignored)
└── README.md                      # build + uso + udev — Fase 4
```

## 7. CONTRATO UI/LOGICA (separación — obligatorio)

El `Injector` NO debe incluir headers de Qt Widgets. Usa signals/slots:

```cpp
// Injector.h (esquema — no es el archivo final, es el contrato)
class Injector : public QObject {
    Q_OBJECT
public:
    explicit Injector(QObject* parent = nullptr);
    ~Injector();

    // Estado
    bool isDeviceConnected() const;

    // Acciones (todas async o con timeout — nunca bloquean la UI)
    void injectPayload(const QString& payloadPath);   // emite payloadInjected / injectionFailed
    void startMonitoring();                            // arranca el worker hotplug
    void stopMonitoring();

signals:
    void deviceConnected();
    void deviceDisconnected();
    void payloadInjected(const QString& payloadPath, int bytesSent);
    void injectionFailed(const QString& errorMessage);
    void logMessage(const QString& message);           // para el log de UI

private:
    class Worker;  // QThread internals
    // ... libusb ctx, handle, hotplug handle, worker thread
};
```

El UI (tabs) se conecta a estas signals. El worker de hotplug es un detalle interno de `Injector`.

## 8. REQUISITOS FUNCIONALES DETALLADOS

### 8.1 Tab Payloads (Fase 1) — MUST
- [ ] Botón "Select payload" → QFileDialog, filtro `*.bin`, recuerda el último directorio (QSettings)
- [ ] QLineEdit (readonly) con la ruta del payload + botón "Inject"
- [ ] Status en status bar: "Waiting for device..." / "Device found (RCM)" / "Injecting..." / "Payload injected (N bytes)"
- [ ] Checkbox "Auto inject when selected" (se inyecta al elegir payload) — Fase 2 wire-up, UI ya en Fase 1
- [ ] Botón "Save as favorite" (deshabilitado hasta Fase 2 o funcional si Favorites ya existe)
- [ ] Los estados de carga: "Loading..." (disable botones), éxito/error (reenable + mensaje)
- [ ] Validación: si no hay device, "Load" muestra error claro "No RCM device detected. Put your Switch in RCM mode."

### 8.2 Tab Favoritos (Fase 2) — MUST
- [ ] QListWidget de payloads guardados
- [ ] Doble click → inyectar directamente
- [ ] Click derecho → menú: "Inject", "Remove"
- [ ] Persistencia: ~/.config/tegrarcm/favorites.json (formato: `{"favorites": ["/path/a.bin", ...]}`)
- [ ] Al inyectar desde favoritos: mismo flujo de status que tab 1

### 8.3 Tab Tools (Fase 3) — SHOULD
- [ ] Botón "Run Linux (ShofEL2)" → inyecta el payload de Linux seleccionado (reusa el mecanismo de inyección)
- [ ] Botón "Mount eMMC/SD (memloader)" → inyecta memloader v3; tras inyectar, detecta el block device nuevo (comparar /dev/sdX antes/después, o udevadm monitor) y muestra la ruta + instrucción de mount
- [ ] Botón "Dump BIS keys" → inyecta biskeydump
- [ ] QPlainTextEdit log de salida (append de cada operación con timestamp)
- [ ] Cada tool necesita su payload .bin: buscar en ~/.config/tegrarcm/payloads/ o pedir al usuario la primera vez (QFileDialog) y recordarlo

### 8.4 Tab Settings (Fase 3) — SHOULD
- [ ] Checkbox "Auto-inject when device connected" (persistente)
- [ ] Checkbox "Minimize to tray" (persistente)
- [ ] Checkbox "Run at startup" → crea/borra ~/.config/autostart/tegrarcm.desktop
- [ ] Botón "Install udev rules" → `pkexec` o `sudo` copiando udev/50-tegrarcm.rules a /etc/udev/rules.d/ + `udevadm control --reload-rules` + `udevadm trigger`; muestra resultado
- [ ] Botón "Open payloads folder" → abre la carpeta de payloads con QDesktopServices

## 9. REQUISITOS NO FUNCIONALES (obligatorios)

- **Sin root para uso normal:** udev rules MODE="0666" → usuario común puede inyectar
- **Código limpio:** separación UI/lógica (sección 7), nombres descriptivos, comentarios SOLO donde explican "por qué" (no "qué")
- **Logging:** qDebug + archivo ~/.local/share/tegrarcm/log.txt (rotación simple: si > 1MB, renombrar a .old)
- **Build:** CMake `find_package(Qt6 COMPONENTS Widgets REQUIRED)` + `pkg_check_modules(LIBUSB REQUIRED IMPORTED_TARGET libusb-1.0)`
- **C++17**, `-Wall -Wextra` sin warnings
- **i18n mínimo:** strings en español (el usuario es argentino); estructura preparada para traducción futura (tr() de Qt)
- **Manejo de errores:** toda llamada libusb chequea return code; errores → signal logMessage + status bar
- **No crashear:** sin device, con device removido a mitad de inyección, con payload inválido (chequear tamaño ≥ 0x1000 y ≤ 1MB), con archivo inexistente

## 10. PLAN DE IMPLEMENTACIÓN POR FASES

### Fase 1 — MVP (EJECUTANDOSE)
- [ ] CMakeLists.txt completo + estructura src/
- [ ] SmashEngine: TegraRcmSmash.cpp adaptado a libusb (mapeo sección 5.2)
- [ ] MainWindow: 4 tabs (Payloads real, otros placeholder) + status bar
- [ ] Tab Payloads: select + inject + status + validaciones
- [ ] Hotplug: worker QThread (gotcha #2) → deviceConnected/Disconnected → status bar
- [ ] Build: `cmake -B build && cmake --build build -j$(nproc)` → binario `tegrarcm-gui`
- [ ] Prueba manual sin hardware: abre, no crashea, muestra "Waiting for device"
- [ ] Prueba con hardware (cuando Axel conecte el Switch): device found + inyección real

### Fase 2 — Favoritos + Auto-inject + Tray
- [ ] Favorites.h/.cpp + tab Favoritos completo (8.2)
- [ ] Auto-inject cuando device conecta (settings persistente)
- [ ] Tray icon (QSystemTrayIcon): show/hide, "Inject last payload", "Quit"
- [ ] Minimize to tray (interceptar closeEvent → hide si está activado)

### Fase 3 — Tools + Settings + Udev
- [ ] Tab Tools completo (8.3)
- [ ] Tab Settings completo (8.4) + persistencia (QSettings: ~/.config/tegrarcm/tegrarcm.conf)
- [ ] UdevRules: udev/50-tegrarcm.rules + instalación con pkexec/sudo
- [ ] Logging a archivo con rotación

### Fase 4 — Pulido + Packaging
- [ ] README completo: build, install udev, uso, troubleshooting (FAQ: "no detecta device" → check udev, cable, unidad patcheada)
- [ ] Icono de la app (assets/icon.png, incluido en el binario vía QResource o ruta relativa)
- [ ] Prueba integral con hardware real (Switch unpatched de Axel)
- [ ] Opcional: CPack/AppImage

## 11. CRITERIOS DE ACEPTACIÓN (por fase — verificables por comando)

### Fase 1
- [ ] `cmake -B build && cmake --build build -j$(nproc)` → exit 0, sin warnings
- [ ] `./build/tegrarcm-gui` abre, muestra 4 tabs, status bar con "Waiting for device..."
- [ ] Con Switch en RCM: status cambia a "Device found (RCM)" en < 1s
- [ ] Select payload + Inject → el Switch bootea (prueba hardware)
- [ ] Sin Switch: la app no crashea, inyectar muestra error claro
- [ ] Cerrar la app no deja threads colgados (exit limpio)

### Fase 2
- [ ] Favoritos persisten entre sesiones (crear, reiniciar app, siguen)
- [ ] Auto-inject funciona al conectar el Switch
- [ ] Tray: minimizar, restaurar, quitar

### Fase 3
- [ ] udev install: corre con pkexec, regla presente, udevadm reload, sin root la inyección funciona
- [ ] ShofEL2/memloader/biskeydump: inyectan el payload correcto (verificación por log + comportamiento del Switch)
- [ ] Startup toggle crea/borra el .desktop

## 12. ANTI-PATTERNS Y PROHIBICIONES (violar esto = rework)

1. **NO modificar archivos en TegraRcmGUI/** (el original) — solo lectura para referencia
2. **NO reescribir el motor** SmashEngine desde cero — el protocolo ya está implementado, solo se adapta transporte
3. **NO inventar APIs** — verificar contra headers instalados (pkg-config --cflags libusb-1.0, /usr/include/qt6/)
4. **NO bloquear el hilo principal** con libusb_handle_events ni operaciones USB largas
5. **NO hardcodear rutas de usuario** — usar QStandardPaths / QDir::homePath
6. **NO dejar memoria cruda sin RAII** — usar QScopedPointer/unique_ptr para ctx/handle libusb
7. **NO usar system() para copiar udev** — usar QProcess con pkexec/sudo (el prompt gráfico de auth es parte del UX)
8. **NO ignorar return codes** de libusb — cada una se chequea, los errores se propagan como signal
9. **NO asumir que el device aparece al instante** — hotplug/polling con retry
10. **NO comentar el código obvio** — comentar el "por qué", nunca el "qué"

## 13. FORMATO DEL REPORTE FINAL (obligatorio al terminar cada fase)

```
## Reporte de implementación — Fase N

### Qué se construyó
(archivos creados/modificados, con una línea de propósito cada uno)

### Cambios sobre el original
(cuando aplique: qué se adaptó en SmashEngine y POR QUÉ — tabla si es posible)

### Decisiones de diseño tomadas
(cada una con su justificación. Si una decisión difiere del spec, marcala como DEVIACIÓN y explicá)

### Build y verificación
(salida real de cmake --build, resultados de las pruebas manuales que pudiste correr)

### Pendiente / riesgos
(qué no se pudo probar sin hardware, qué puede fallar en la integración)

### Próximo paso sugerido
(una línea — qué fase sigue y con qué foco)
```

## 14. RESTRICCIONES PARA CLAUDE CODE (resumen ejecutable)

- Trabajar SOLO en: src/, CMakeLists.txt, udev/, README.md, assets/
- Leer (nunca modificar): TegraRcmGUI/ (original)
- Copiar TegraRcmSmash.cpp → src/SmashEngine.cpp y adaptar; documentar cambios en comentario al inicio del archivo
- Mantener headers GPL-2.0 en archivos derivados
- Cada fase termina con: build OK + reporte (sección 13)
- Si algo del spec es ambiguo: decidir, documentar como DEVIACIÓN, seguir — NO preguntar (modo autónomo)
