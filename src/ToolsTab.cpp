/*
 * ToolsTab.cpp
 * Part of TegraRcmGUI-Linux, a Linux port of eliboa/TegraRcmGUI.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "ToolsTab.h"
#include "Injector.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>

namespace {
constexpr const char *kShofel2PathKey = "tools/shofel2Path";
constexpr const char *kBiskeydumpPathKey = "tools/biskeydumpPath";
constexpr const char *kLockpickPathKey = "tools/lockpickPath";

// Payloads por defecto para cada herramienta (instalados por install.sh en
// ~/.local/share/tegrarcm/payloads/tools/ o junto al binario en ../payloads/tools/).
constexpr const char *kShofel2DefaultPayload = "shofel2_coreboot.rom";
constexpr const char *kBiskeydumpDefaultPayload = "biskeydump_usb.bin";
constexpr const char *kLockpickDefaultPayload = "lockpick_rcm.bin";

QString defaultToolPayload(const char *fileName)
{
    // 1) payloads del release instalado
    const QDir installed(QDir::homePath() + QStringLiteral("/.local/share/tegrarcm/payloads/tools"));
    if (QFileInfo::exists(installed.filePath(QString::fromLatin1(fileName)))) {
        return installed.filePath(QString::fromLatin1(fileName));
    }
    // 2) junto al binario (build tree / carpeta portable)
    const QString appDir = QCoreApplication::applicationDirPath();
    const QStringList candidates = {
        appDir + QStringLiteral("/../payloads/tools/") + QString::fromLatin1(fileName),
        appDir + QStringLiteral("/payloads/tools/") + QString::fromLatin1(fileName),
    };
    for (const QString &candidate : candidates) {
        if (QFileInfo::exists(candidate)) {
            return candidate;
        }
    }
    return {};
}
}

ToolsTab::ToolsTab(Injector *injector, QWidget *parent)
    : QWidget(parent)
    , m_injector(injector)
    , m_log(new QPlainTextEdit(this))
{
    m_shofel2.settingsKey = QString::fromLatin1(kShofel2PathKey);
    m_shofel2.displayName = tr("Linux (ShofEL2)");
    m_shofel2.defaultPayload = QString::fromLatin1(kShofel2DefaultPayload);
    m_biskeydump.settingsKey = QString::fromLatin1(kBiskeydumpPathKey);
    m_biskeydump.displayName = tr("biskeydump (claves BIS)");
    m_biskeydump.defaultPayload = QString::fromLatin1(kBiskeydumpDefaultPayload);
    m_lockpick.settingsKey = QString::fromLatin1(kLockpickPathKey);
    m_lockpick.displayName = tr("Lockpick_RCM (claves)");
    m_lockpick.defaultPayload = QString::fromLatin1(kLockpickDefaultPayload);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(makeToolGroup(m_shofel2, tr("Correr Linux"), tr("Run Linux (ShofEL2)")));
    layout->addWidget(makeToolGroup(
        m_biskeydump, tr("Volcar claves BIS"), tr("Dump BIS keys")));
    layout->addWidget(makeToolGroup(
        m_lockpick, tr("Volcar claves (Lockpick_RCM)"), tr("Dump keys (Lockpick_RCM)")));

    m_log->setReadOnly(true);
    m_log->setMaximumBlockCount(2000);
    layout->addWidget(new QLabel(tr("Registro:"), this));
    layout->addWidget(m_log, 1);

    connect(m_injector, &Injector::logMessage, this, &ToolsTab::onLogMessage);
    connect(m_injector, &Injector::payloadInjected, this, &ToolsTab::onPayloadInjected);
    connect(m_injector, &Injector::injectionFailed, this, &ToolsTab::onInjectionFailed);
}

QWidget *ToolsTab::makeToolGroup(Tool &tool, const QString &groupTitle, const QString &runLabel)
{
    auto *group = new QGroupBox(groupTitle, this);
    auto *groupLayout = new QVBoxLayout(group);

    auto *pathRow = new QHBoxLayout;
    tool.pathLabel = new QLabel(tr("Sin payload seleccionado"), group);
    tool.pathLabel->setWordWrap(true);
    auto *changeButton = new QPushButton(tr("Cambiar payload..."), group);
    pathRow->addWidget(tool.pathLabel, 1);
    pathRow->addWidget(changeButton);

    tool.runButton = new QPushButton(runLabel, group);

    groupLayout->addLayout(pathRow);
    groupLayout->addWidget(tool.runButton);

    const QSettings settings;
    QString savedPath = settings.value(tool.settingsKey).toString();
    if (savedPath.isEmpty() || !QFileInfo::exists(savedPath)) {
        // Payload por defecto de la herramienta si no hay uno guardado/válido.
        savedPath = defaultToolPayload(tool.defaultPayload.toLatin1().constData());
        if (!savedPath.isEmpty()) {
            tool.pathLabel->setText(savedPath);
            QSettings writable;
            writable.setValue(tool.settingsKey, savedPath);
        }
    }
    if (!savedPath.isEmpty() && QFileInfo::exists(savedPath)) {
        tool.pathLabel->setText(savedPath);
    }

    connect(changeButton, &QPushButton::clicked, this, [this, &tool]() {
        const QString path = QFileDialog::getOpenFileName(
            this, tr("Seleccionar payload"), QString(), tr("Bin files (*.bin);;All files (*)"));
        if (path.isEmpty()) {
            return;
        }
        setPath(tool, path);
    });

    connect(tool.runButton, &QPushButton::clicked, this, [this, &tool]() { runTool(tool); });

    return group;
}

void ToolsTab::setPath(Tool &tool, const QString &path)
{
    tool.pathLabel->setText(path);
    QSettings settings;
    settings.setValue(tool.settingsKey, path);
}

void ToolsTab::runTool(Tool &tool)
{
    QSettings settings;
    QString path = settings.value(tool.settingsKey).toString();
    if (path.isEmpty() || !QFileInfo::exists(path)) {
        // Default de la herramienta antes de pedir el dialogo.
        path = defaultToolPayload(tool.defaultPayload.toLatin1().constData());
    }
    if (path.isEmpty() || !QFileInfo::exists(path)) {
        path = QFileDialog::getOpenFileName(
            this, tr("Seleccionar payload para %1").arg(tool.displayName), QString(),
            tr("Bin files (*.bin);;All files (*)"));
        if (path.isEmpty()) {
            return;
        }
        setPath(tool, path);
    } else if (settings.value(tool.settingsKey).toString().isEmpty()) {
        setPath(tool, path);
    }

    appendLog(tr("%1: inyectando %2").arg(tool.displayName, path));
    setButtonsEnabled(false);
    m_injector->injectPayload(path);
}

void ToolsTab::setButtonsEnabled(bool enabled)
{
    m_shofel2.runButton->setEnabled(enabled);
    m_biskeydump.runButton->setEnabled(enabled);
    m_lockpick.runButton->setEnabled(enabled);
}

void ToolsTab::appendLog(const QString &message)
{
    const QString timestamp = QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss"));
    m_log->appendPlainText(QStringLiteral("[%1] %2").arg(timestamp, message));
}

void ToolsTab::onLogMessage(const QString &message)
{
    appendLog(message);
}

void ToolsTab::onPayloadInjected(const QString &path, int bytesSent)
{
    Q_UNUSED(bytesSent)
    appendLog(tr("Payload inyectado: %1").arg(path));
    setButtonsEnabled(true);
}

void ToolsTab::onInjectionFailed(const QString &errorMessage)
{
    appendLog(tr("Error: %1").arg(errorMessage));
    setButtonsEnabled(true);
}
