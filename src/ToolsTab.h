/*
 * ToolsTab.h
 * Part of TegraRcmGUI-Linux, a Linux port of eliboa/TegraRcmGUI.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Each tool remembers its payload path in QSettings ("tools/<tool>Path"),
 * asking via QFileDialog only the first time (or if the remembered file
 * disappeared). Injection itself is delegated entirely to the shared
 * Injector instance passed in by MainWindow — see SPEC.md section 8.3.
 */
#pragma once

#include <QWidget>

class Injector;
class QLabel;
class QPlainTextEdit;
class QPushButton;

class ToolsTab : public QWidget
{
    Q_OBJECT

public:
    explicit ToolsTab(Injector *injector, QWidget *parent = nullptr);

private slots:
    void onLogMessage(const QString &message);
    void onPayloadInjected(const QString &path, int bytesSent);
    void onInjectionFailed(const QString &errorMessage);

private:
    struct Tool {
        QString settingsKey;
        QString displayName;
        QString defaultPayload;  // nombre del payload por defecto (payloads/tools/)
        QLabel *pathLabel = nullptr;
        QPushButton *runButton = nullptr;
    };

    QWidget *makeToolGroup(Tool &tool, const QString &groupTitle, const QString &runLabel);
    void setPath(Tool &tool, const QString &path);
    void runTool(Tool &tool);
    void setButtonsEnabled(bool enabled);
    void appendLog(const QString &message);

    Injector *m_injector;
    Tool m_shofel2;
    Tool m_biskeydump;
    QPlainTextEdit *m_log;
};
