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

#include <libusb-1.0/libusb.h>

// Ruta al motor de inyeccion nativo (launcher_rcm — replica del motor de NXLoader,
// C puro con ioctls USBDEVFS; el port python del fusee-launcher quedo como referencia).
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

// Ruta al motor de inyeccion python (fusee-launcher, ktemkin/fail0verflow, GPL-2.0).
// Mismo diseno que el original: el GUI invoca un binario externo que hace
// la inyeccion USB real (original: TegraRcmSmash.exe; port: fusee-launcher.py).
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
                                "la deteccion de dispositivo quedara inactiva."));
        }

        while (!m_stopRequested.load()) {
            struct timeval timeout{};
            timeout.tv_sec = 0;
            timeout.tv_usec = 100000; // 100ms, per SPEC gotcha #2
            libusb_handle_events_timeout_completed(m_ctx, &timeout, nullptr);
        }

        if (callbackRegistered) {
            libusb_hotplug_deregister_callback(m_ctx, callbackHandle);
        }
    }

private:
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
        emit injectionFailed(QStringLiteral("Ya hay una inyeccion en curso."));
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

    // Motor nativo: launcher_rcm (replica del motor de NXLoader).
    const QString launcherPath = rcmLauncherPath();
    const QFileInfo launcherInfo(launcherPath);
    if (!launcherInfo.exists()) {
        emit injectionFailed(
            QStringLiteral("Motor de inyeccion no encontrado: %1").arg(launcherPath));
        return;
    }

    // Intermezzo (relocator) al lado del motor — el de 92 bytes (par NXLoader).
    const QString relocatorPath =
        QFileInfo(launcherInfo.absolutePath() + QStringLiteral("/intermezzo.bin"))
            .absoluteFilePath();
    if (!QFileInfo(relocatorPath).exists()) {
        emit injectionFailed(
            QStringLiteral("Intermezzo no encontrado junto al motor: %1").arg(relocatorPath));
        return;
    }

    // Resolver el device path (/dev/bus/usb/BBB/DDD) del Tegra en RCM via libusb.
    QString devicePath;
    libusb_device **devs = nullptr;
    const ssize_t cnt = libusb_get_device_list(m_ctx, &devs);
    for (ssize_t i = 0; i < cnt; ++i) {
        libusb_device_descriptor desc;
        if (libusb_get_device_descriptor(devs[i], &desc) == LIBUSB_SUCCESS &&
            desc.idVendor == kRcmVendorId && desc.idProduct == kRcmProductId) {
            devicePath = QStringLiteral("/dev/bus/usb/%1/%2")
                             .arg(libusb_get_bus_number(devs[i]), 3, 10, QLatin1Char('0'))
                             .arg(libusb_get_device_address(devs[i]), 3, 10, QLatin1Char('0'));
            break;
        }
    }
    if (devs != nullptr) {
        libusb_free_device_list(devs, 1);
    }
    if (devicePath.isEmpty()) {
        emit injectionFailed(
            QStringLiteral("Dispositivo RCM no encontrado. Conecta la Switch en modo RCM."));
        return;
    }

    emit logMessage(QStringLiteral("Inyectando payload: %1").arg(payloadPath));

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
