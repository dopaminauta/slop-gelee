/*
 * UdevRules.cpp
 * Part of TegraRcmGUI-Linux, a Linux port of eliboa/TegraRcmGUI.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "UdevRules.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QStandardPaths>

// Ruta a udev/50-tegrarcm.rules, buscada igual que fusee-launcher.py en
// Injector.cpp: junto al arbol de build (../udev) o junto al binario instalado.
QString UdevRules::rulesSourcePath()
{
    const QString appDir = QCoreApplication::applicationDirPath();
    const QStringList candidates = {
        appDir + QStringLiteral("/../udev/50-tegrarcm.rules"),
        appDir + QStringLiteral("/udev/50-tegrarcm.rules"),
    };
    for (const QString &candidate : candidates) {
        if (QFileInfo::exists(candidate)) {
            return candidate;
        }
    }
    return candidates.first();
}

UdevRules::UdevRules(QObject *parent) : QObject(parent)
{
}

void UdevRules::install()
{
    if (m_process != nullptr) {
        emit logMessage(tr("Ya hay una instalacion en curso."));
        return;
    }

    const QString source = rulesSourcePath();
    if (!QFileInfo::exists(source)) {
        emit logMessage(tr("No se encontro udev/50-tegrarcm.rules en: %1").arg(source));
        emit finished(false);
        return;
    }

    // Un solo pkexec/sudo para las tres operaciones: evita pedir autenticacion
    // tres veces. El comando no incluye entrada de usuario, solo esta ruta fija.
    const QString command = QStringLiteral(
        "cp '%1' /etc/udev/rules.d/50-tegrarcm.rules && "
        "udevadm control --reload-rules && udevadm trigger").arg(source);

    emit logMessage(tr("Installing udev rules (authentication required)..."));

    m_process = new QProcess(this);
    connect(m_process, &QProcess::finished, this, &UdevRules::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred, this, &UdevRules::onProcessError);

    const QString pkexecPath = QStandardPaths::findExecutable(QStringLiteral("pkexec"));
    const QString elevator = pkexecPath.isEmpty() ? QStringLiteral("sudo") : pkexecPath;
    m_process->setProgram(elevator);
    m_process->setArguments({QStringLiteral("sh"), QStringLiteral("-c"), command});
    m_process->start();
}

void UdevRules::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (m_process == nullptr) {
        return;
    }

    const QString output = QString::fromLocal8Bit(m_process->readAllStandardOutput());
    const QString error = QString::fromLocal8Bit(m_process->readAllStandardError());
    const bool success = (exitStatus == QProcess::NormalExit) && (exitCode == 0);

    if (success) {
        emit logMessage(tr("udev rules installed successfully."));
        if (!output.trimmed().isEmpty()) {
            emit logMessage(output.trimmed());
        }
    } else {
        QString detail = error.trimmed().isEmpty() ? output.trimmed() : error.trimmed();
        if (detail.isEmpty()) {
            detail = tr("sin detalle (pkexec/sudo cancelado o denegado)");
        }
        emit logMessage(tr("Instalacion cancelada o fallida (exit %1): %2")
                             .arg(exitCode)
                             .arg(detail));
    }

    emit finished(success);
    m_process->deleteLater();
    m_process = nullptr;
}

void UdevRules::onProcessError(QProcess::ProcessError error)
{
    if (m_process == nullptr) {
        return;
    }
    emit logMessage(tr("Process error (%1): %2")
                         .arg(static_cast<int>(error))
                         .arg(m_process->errorString()));
    emit finished(false);
    m_process->deleteLater();
    m_process = nullptr;
}
