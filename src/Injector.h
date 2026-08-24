/*
 * Injector.h
 * Part of TegraRcmGUI-Linux, a Linux port of eliboa/TegraRcmGUI.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * libusb-1.0 wrapper: device detection (VID 0x0955 / PID 0x7321, NVIDIA
 * APX in RCM) via hotplug, and (from phase 1.5 onward) payload injection.
 * See SPEC.md section 5 for the libusbk -> libusb-1.0 mapping this class
 * is built against, and section 7 for the UI/logic contract it implements.
 */
#pragma once

#include <QCoreApplication>
#include <QObject>
#include <QProcess>
#include <QString>

#include <atomic>

struct libusb_context;

class Injector : public QObject
{
    Q_OBJECT

public:
    static constexpr int kRcmVendorId = 0x0955;
    static constexpr int kRcmProductId = 0x7321;

    explicit Injector(QObject *parent = nullptr);
    ~Injector() override;

    bool isDeviceConnected() const;

    // Acciones (todas async o con timeout — nunca bloquean la UI)
    void injectPayload(const QString &payloadPath);
    void startMonitoring();
    void stopMonitoring();

signals:
    void deviceConnected();
    void deviceDisconnected();
    void payloadInjected(const QString &payloadPath, int bytesSent);
    void injectionFailed(const QString &errorMessage);
    void logMessage(const QString &message);

private slots:
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);

private:
    class Worker;

    void handleDeviceArrived();
    void handleDeviceLeft();

    libusb_context *m_ctx = nullptr;
    Worker *m_worker = nullptr;
    QProcess *m_process = nullptr;
    std::atomic<bool> m_deviceConnected{false};

    // Injection retries: a freshly connected device may not be ready
    // when hotplug detects it (the winning script waited for full enumeration).
    int m_injectRetries = 0;
    QString m_retryDevicePath;
    QString m_retryPayloadPath;
    QString m_retryRelocatorPath;
};
