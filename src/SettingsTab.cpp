/*
 * SettingsTab.cpp
 * Part of TegraRcmGUI-Linux, a Linux port of eliboa/TegraRcmGUI.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "SettingsTab.h"
#include "UdevRules.h"

#include <QCheckBox>
#include <QCoreApplication>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSettings>
#include <QStandardPaths>
#include <QTextStream>
#include <QUrl>
#include <QVBoxLayout>

SettingsTab::SettingsTab(QWidget *parent)
    : QWidget(parent)
    , m_autoInjectCheck(new QCheckBox(tr("Auto-enviar al conectar el dispositivo"), this))
    , m_minimizeToTrayCheck(new QCheckBox(tr("Minimizar a la bandeja del sistema"), this))
    , m_runAtStartupCheck(new QCheckBox(tr("Iniciar automaticamente con el sistema"), this))
    , m_log(new QPlainTextEdit(this))
    , m_udevRules(new UdevRules(this))
{
    auto *layout = new QVBoxLayout(this);
    layout->addWidget(m_autoInjectCheck);
    layout->addWidget(m_minimizeToTrayCheck);
    layout->addWidget(m_runAtStartupCheck);

    auto *openFolderButton = new QPushButton(tr("Abrir carpeta de payloads"), this);
    layout->addWidget(openFolderButton);

    auto *installUdevButton = new QPushButton(tr("Instalar reglas udev"), this);
    layout->addWidget(installUdevButton);

    m_log->setReadOnly(true);
    m_log->setMaximumBlockCount(2000);
    layout->addWidget(new QLabel(tr("Registro:"), this));
    layout->addWidget(m_log, 1);

    QSettings settings;
    m_autoInjectCheck->setChecked(settings.value(kAutoInjectKey, false).toBool());
    m_minimizeToTrayCheck->setChecked(settings.value(kMinimizeToTrayKey, false).toBool());
    m_runAtStartupCheck->setChecked(QFile::exists(autostartFilePath()));

    connect(m_autoInjectCheck, &QCheckBox::toggled, this, &SettingsTab::onAutoInjectToggled);
    connect(m_minimizeToTrayCheck, &QCheckBox::toggled,
            this, &SettingsTab::onMinimizeToTrayToggled);
    connect(m_runAtStartupCheck, &QCheckBox::toggled, this, &SettingsTab::onRunAtStartupToggled);
    connect(openFolderButton, &QPushButton::clicked, this, &SettingsTab::onOpenPayloadsFolder);
    connect(installUdevButton, &QPushButton::clicked, this, &SettingsTab::onInstallUdevRules);
    connect(m_udevRules, &UdevRules::logMessage, this, &SettingsTab::onUdevLogMessage);
}

void SettingsTab::onAutoInjectToggled(bool checked)
{
    QSettings settings;
    settings.setValue(kAutoInjectKey, checked);
}

void SettingsTab::onMinimizeToTrayToggled(bool checked)
{
    QSettings settings;
    settings.setValue(kMinimizeToTrayKey, checked);
}

void SettingsTab::onRunAtStartupToggled(bool checked)
{
    if (checked) {
        writeAutostartFile();
    } else {
        removeAutostartFile();
    }
}

void SettingsTab::onOpenPayloadsFolder()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
        + QStringLiteral("/tegrarcm/payloads");
    QDir().mkpath(dir);
    QDesktopServices::openUrl(QUrl::fromLocalFile(dir));
}

void SettingsTab::onInstallUdevRules()
{
    m_udevRules->install();
}

void SettingsTab::onUdevLogMessage(const QString &message)
{
    const QString timestamp = QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss"));
    m_log->appendPlainText(QStringLiteral("[%1] %2").arg(timestamp, message));
}

QString SettingsTab::autostartFilePath()
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
        + QStringLiteral("/autostart/tegrarcm.desktop");
}

void SettingsTab::writeAutostartFile()
{
    const QString path = autostartFilePath();
    QDir().mkpath(QFileInfo(path).absolutePath());

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return;
    }

    QTextStream out(&file);
    out << "[Desktop Entry]\n"
        << "Type=Application\n"
        << "Name=TegraRcmGUI\n"
        << "Exec=" << QCoreApplication::applicationFilePath() << "\n"
        << "Icon=tegrarcm-gui\n"
        << "Terminal=false\n"
        << "X-GNOME-Autostart-enabled=true\n";
}

void SettingsTab::removeAutostartFile()
{
    QFile::remove(autostartFilePath());
}
