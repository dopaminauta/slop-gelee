/*
 * SettingsTab.h
 * Part of TegraRcmGUI-Linux, a Linux port of eliboa/TegraRcmGUI.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Settings are persisted via QSettings (org "tegrarcm", app "tegrarcm-gui"),
 * which resolves to ~/.config/tegrarcm/tegrarcm-gui.conf on Linux.
 */
#pragma once

#include <QWidget>

class QCheckBox;
class QPlainTextEdit;
class UdevRules;

class SettingsTab : public QWidget
{
    Q_OBJECT

public:
    // QSettings keys shared with MainWindow (auto-inject wiring, tray close behavior).
    static constexpr const char *kAutoInjectKey = "autoInject";
    static constexpr const char *kMinimizeToTrayKey = "minimizeToTray";

    explicit SettingsTab(QWidget *parent = nullptr);

private slots:
    void onAutoInjectToggled(bool checked);
    void onMinimizeToTrayToggled(bool checked);
    void onRunAtStartupToggled(bool checked);
    void onOpenPayloadsFolder();
    void onInstallUdevRules();
    void onUdevLogMessage(const QString &message);

private:
    static QString autostartFilePath();
    static void writeAutostartFile();
    static void removeAutostartFile();

    QCheckBox *m_autoInjectCheck;
    QCheckBox *m_minimizeToTrayCheck;
    QCheckBox *m_runAtStartupCheck;
    QPlainTextEdit *m_log;
    UdevRules *m_udevRules;
};
