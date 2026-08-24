/*
 * Injector.cpp
 * Part of TegraRcmGUI-Linux, a Linux port of eliboa/TegraRcmGUI.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * SPEC.md 5.2 gotcha #2: libusb_handle_events() must never run on the Qt
 * main thread (it would block the event loop). This file registers a
 * hotplug callback (VID 0x0955 / PID 0x7321) and pumps it from a QThread
 * worker that loops on libusb_handle_events_timeout_completed() with a
 * short timeout, checking a stop flag each iteration. Chosen over the
 * QTimer-polling alternative because the device is only ever present in
 * RCM mode, so event-driven detection reacts faster than a fixed-period
 * poll while costing nothing extra now that the worker thread exists.
 */
#include "Injector.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QProcess>
#include <QThread>
#include <QTimer>

#include <libusb-1.0/libusb.h>

// Path to the native injection engine (launcher_rcm — a replica of the NXLoader engine,
// plain C with USBDEVFS ioctls; the python fusee-launcher port is kept as reference).
// Se busca en ../tools/ (build tree) y junto al binario (instalado).
static QString rcmLauncherPath()
{
    const QString appDir = QCoreApplication::applicationDirPath();
    const QStringList candidates = {
        appDir + QStringLiteral("/../tools/launcher_rcm"),
        appDir + QStringLiteral("/tools/launcher_rcm"),
    };
    for (const QString &candidate : candidates) {
        if (QFileInfo::exists(candidate)) {
            return candidate;
        }
    }
    return candidates.first();
}

// Path to the python injection engine (fusee-launcher, ktemkin/fail0verflow, GPL-2.0).
// Same design as the original: the GUI invokes an external binary that does
// the real USB injection (original: TegraRcmSmash.exe; port: fusee-launcher.py).
// Se busca en ../tools/ (build tree) y junto al binario (instalado).
static QString fuseeLauncherPath()
{
    const QString appDir = QCoreApplication::applicationDirPath();
    const QStringList candidates = {
        appDir + QStringLiteral("/../tools/fusee-launcher.py"),
        appDir + QStringLiteral("/tools/fusee-launcher.py"),
    };
    for (const QString &candidate : candidates) {
        if (QFileInfo::exists(candidate)) {
            return candidate;
        }
    }
    return candidates.first();
}

class Injector::Worker : public QThread
{
public:
    Worker(Injector *owner, libusb_context *ctx)
        : m_owner(owner), m_ctx(ctx)
    {
    }

    void requestStop()
    {
        m_stopRequested.store(true);
        if (m_ctx != nullptr) {
            libusb_interrupt_event_handler(m_ctx);
        }
    }

protected:
    void run() override
    {
        const bool hotplugAvailable = libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG) != 0;
        bool callbackRegistered = false;
        libusb_hotplug_callback_handle callbackHandle = 0;

        if (hotplugAvailable) {
            const int rc = libusb_hotplug_register_callback(
                m_ctx,
                static_cast<libusb_hotplug_event>(LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED
                                                   | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT),
                LIBUSB_HOTPLUG_ENUMERATE,
                Injector::kRcmVendorId,
                Injector::kRcmProductId,
                LIBUSB_HOTPLUG_MATCH_ANY,
                &Worker::hotplugCallback,
                this,
                &callbackHandle);

            if (rc == LIBUSB_SUCCESS) {
                callbackRegistered = true;
            } else {
                emit m_owner->logMessage(
                    QStringLiteral("No se pudo registrar hotplug callback: %1")
                        .arg(libusb_error_name(rc)));
            }
        } else {
            emit m_owner->logMessage(
                QStringLiteral("libusb no soporta hotplug en este sistema; "
                                "device detection is inactive."));
        }

        while (!m_stopRequested.load()) {
            struct timeval timeout{};
            timeout.tv_sec = 0;
            timeout.tv_usec = 100000; // 100ms, per SPEC gotcha #2
            libusb_handle_events_timeout_completed(m_ctx, &timeout, nullptr);

            // Polling defensivo de la lista (cada ~1s): el hotplug de libusb se
            // pierde los devices re-enumerados por el kernel (unbind/reset) y los
            // devices conectados antes de abrir la app. El script ganador usaba
            // polling de lsusb — esto replica eso dentro de la GUI.
            if (++m_tick % 10 == 0) {
                const bool present = scanForRcmDevice();
                if (present != m_owner->isDeviceConnected()) {
                    if (present) {
                        m_owner->handleDeviceArrived();
                    } else {
                        m_owner->handleDeviceLeft();
                    }
                }
            }
        }

        if (callbackRegistered) {
            libusb_hotplug_deregister_callback(m_ctx, callbackHandle);
        }
    }

private:
    // Escaneo de la lista actual de devices en busca del Tegra en RCM.
    // Devuelve true si hay AL MENOS uno presente.
    bool scanForRcmDevice()
    {
        libusb_device **devs = nullptr;
        const ssize_t cnt = libusb_get_device_list(m_ctx, &devs);
        bool found = false;
        for (ssize_t i = 0; i < cnt && !found; ++i) {
            libusb_device_descriptor desc;
            if (libusb_get_device_descriptor(devs[i], &desc) == LIBUSB_SUCCESS &&
                desc.idVendor == Injector::kRcmVendorId &&
                desc.idProduct == Injector::kRcmProductId) {
                found = true;
            }
        }
        if (devs != nullptr) {
            libusb_free_device_list(devs, 1);
        }
        return found;
    }

    int m_tick = 0;
    static int LIBUSB_CALL hotplugCallback(libusb_context *ctx, libusb_device *device,
                                            libusb_hotplug_event event, void *userData)
    {
        Q_UNUSED(ctx)
        Q_UNUSED(device)

        auto *self = static_cast<Worker *>(userData);
        if (event == LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED) {
            self->m_owner->handleDeviceArrived();
        } else if (event == LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT) {
            self->m_owner->handleDeviceLeft();
        }
        return 0; // 0 = keep the callback registered
    }

    Injector *m_owner;
    libusb_context *m_ctx;
    std::atomic<bool> m_stopRequested{false};
};

Injector::Injector(QObject *parent)
    : QObject(parent)
{
    const int rc = libusb_init(&m_ctx);
    if (rc != LIBUSB_SUCCESS) {
        emit logMessage(QStringLiteral("libusb_init fallo: %1").arg(libusb_error_name(rc)));
        m_ctx = nullptr;
        return;
    }
    libusb_set_option(m_ctx, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_WARNING);
}

Injector::~Injector()
{
    stopMonitoring();
    if (m_ctx != nullptr) {
        libusb_exit(m_ctx);
        m_ctx = nullptr;
    }
}

bool Injector::isDeviceConnected() const
{
    return m_deviceConnected.load();
}

void Injector::startMonitoring()
{
    if (m_ctx == nullptr || m_worker != nullptr) {
        return;
    }
    m_worker = new Worker(this, m_ctx);
    m_worker->start();
}

void Injector::stopMonitoring()
{
    if (m_worker == nullptr) {
        return;
    }
    m_worker->requestStop();
    m_worker->wait();
    delete m_worker;
    m_worker = nullptr;
}

void Injector::injectPayload(const QString &payloadPath)
{
    if (m_process != nullptr) {
        emit injectionFailed(QStringLiteral("Injection already in progress."));
        return;
    }

    const QFileInfo fileInfo(payloadPath);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        emit injectionFailed(QStringLiteral("El archivo no existe: %1").arg(payloadPath));
        return;
    }
    const qint64 size = fileInfo.size();
    if (size < 0x1000 || size > 1024 * 1024) {
        emit injectionFailed(
            QStringLiteral("Tamano de payload invalido (%1 bytes; se espera 4096..1048576).")
                .arg(size));
        return;
    }

    // Native engine: launcher_rcm (replica of the NXLoader engine).
    const QString launcherPath = rcmLauncherPath();
    const QFileInfo launcherInfo(launcherPath);
    if (!launcherInfo.exists()) {
        emit injectionFailed(
            QStringLiteral("Injection engine not found: %1").arg(launcherPath));
        return;
    }

    // Intermezzo (relocator) next to the engine — the 92-byte one (NXLoader pair).
    const QString relocatorPath =
        QFileInfo(launcherInfo.absolutePath() + QStringLiteral("/intermezzo.bin"))
            .absoluteFilePath();
    if (!QFileInfo(relocatorPath).exists()) {
        emit injectionFailed(
            QStringLiteral("Intermezzo not found next to the engine: %1").arg(relocatorPath));
        return;
    }

    // Resolver el device path (/dev/bus/usb/BBB/DDD) del Tegra en RCM via libusb.
    // IMPORTANTE: elegir el device de direccion MAS ALTA (el mas nuevo). Los
    // devices quemados quedan enumerados y el primero de la lista puede ser uno
    // muerto — el RCM recien conectado siempre tiene el numero mas alto.
    QString devicePath;
    int bestBus = -1;
    int bestAddr = -1;
    libusb_device **devs = nullptr;
    const ssize_t cnt = libusb_get_device_list(m_ctx, &devs);
    for (ssize_t i = 0; i < cnt; ++i) {
        libusb_device_descriptor desc;
        if (libusb_get_device_descriptor(devs[i], &desc) == LIBUSB_SUCCESS &&
            desc.idVendor == kRcmVendorId && desc.idProduct == kRcmProductId) {
            const int bus = libusb_get_bus_number(devs[i]);
            const int addr = libusb_get_device_address(devs[i]);
            if (addr > bestAddr || (addr == bestAddr && bus > bestBus)) {
                bestBus = bus;
                bestAddr = addr;
                devicePath = QStringLiteral("/dev/bus/usb/%1/%2")
                                 .arg(bus, 3, 10, QLatin1Char('0'))
                                 .arg(addr, 3, 10, QLatin1Char('0'));
            }
        }
    }
    if (devs != nullptr) {
        libusb_free_device_list(devs, 1);
    }
    if (devicePath.isEmpty()) {
        emit injectionFailed(
            QStringLiteral("RCM device not found. Connect the Switch in RCM mode."));
        return;
    }

    emit logMessage(QStringLiteral("Injecting payload: %1").arg(payloadPath));

    // El hotplug de libusb se dispara antes de que udev cree el node del device;
    // esperar un instante para que /dev/bus/usb/BBB/DDD exista (el RCM se agota
    // en ~60s, 400ms no comprometen la ventana).
    QThread::msleep(400);

    m_retryDevicePath = devicePath;
    m_retryPayloadPath = payloadPath;
    m_retryRelocatorPath = relocatorPath;
    m_injectRetries = 0;

    m_process = new QProcess(this);
    connect(m_process, &QProcess::finished, this, &Injector::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred, this, &Injector::onProcessError);

    m_process->setProgram(launcherPath);
    m_process->setArguments({devicePath, payloadPath, relocatorPath});
    m_process->start();
}

void Injector::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (m_process == nullptr) {
        return;
    }

    const QString output = QString::fromLocal8Bit(m_process->readAllStandardOutput());
    const QString error = QString::fromLocal8Bit(m_process->readAllStandardError());

    // fusee-launcher: exit 0 = payload enviado; codigos negativos = error
    const bool success = (exitStatus == QProcess::NormalExit) && (exitCode == 0);
    if (success) {
        emit logMessage(output.trimmed());
        emit payloadInjected(m_process->program(), 0 /* bytes; fusee-launcher no los reporta */);
    } else {
        const QString detail = error.trimmed().isEmpty() ? output.trimmed() : error.trimmed();

        // Reintento automatico si el device no estaba listo (read timeout) y
        // el device sigue presente — el RCM fresco a veces tarda en responder.
        const bool retriable = detail.contains(QStringLiteral("fallo al leer device ID")) ||
                               detail.contains(QStringLiteral("no se pudo abrir"));
        if (retriable && m_injectRetries < 3 && isDeviceConnected()) {
            ++m_injectRetries;
            emit logMessage(QStringLiteral("Retrying (%1/3)...").arg(m_injectRetries));
            m_process->deleteLater();
            m_process = nullptr;
            QTimer::singleShot(600, this, [this]() {
                injectPayload(m_retryPayloadPath);
            });
            return;
        }

        emit injectionFailed(
            QStringLiteral("Error al inyectar (exit %1): %2").arg(exitCode).arg(detail));
    }

    m_process->deleteLater();
    m_process = nullptr;
}

void Injector::onProcessError(QProcess::ProcessError error)
{
    if (m_process == nullptr) {
        return;
    }
    emit injectionFailed(QStringLiteral("Error de proceso (%1): %2")
                             .arg(static_cast<int>(error))
                             .arg(m_process->errorString()));
    m_process->deleteLater();
    m_process = nullptr;
}

void Injector::handleDeviceArrived()
{
    if (!m_deviceConnected.exchange(true)) {
        emit deviceConnected();
    }
}

void Injector::handleDeviceLeft()
{
    if (m_deviceConnected.exchange(false)) {
        emit deviceDisconnected();
    }
}
