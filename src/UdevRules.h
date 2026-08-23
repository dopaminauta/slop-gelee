/*
 * UdevRules.h
 * Part of TegraRcmGUI-Linux, a Linux port of eliboa/TegraRcmGUI.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Installs udev/50-tegrarcm.rules to /etc/udev/rules.d/ via pkexec (falls
 * back to sudo) so that unprivileged users can access the APX device
 * (see SPEC.md section 8.4 and section 12 point 7: no system(), QProcess only).
 */
#pragma once

#include <QObject>
#include <QProcess>

class UdevRules : public QObject
{
    Q_OBJECT

public:
    explicit UdevRules(QObject *parent = nullptr);

    void install();

signals:
    void logMessage(const QString &message);
    void finished(bool success);

private slots:
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);

private:
    static QString rulesSourcePath();

    QProcess *m_process = nullptr;
};
