/*
 * main.cpp
 * Part of TegraRcmGUI-Linux, a Linux port of eliboa/TegraRcmGUI.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "MainWindow.h"

#include <QApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QIcon>
#include <QMutex>
#include <QMutexLocker>
#include <QStandardPaths>
#include <QTextStream>

namespace {

QString logFilePath()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
        + QStringLiteral("/tegrarcm");
    QDir().mkpath(dir);
    return dir + QStringLiteral("/log.txt");
}

// Rotacion simple: si el log supera 1MB se renombra a .old antes de escribir,
// para no crecer sin limite en sesiones largas (SPEC.md seccion 9).
void rotateLogIfNeeded(const QString &path)
{
    const QFileInfo info(path);
    if (info.exists() && info.size() > 1024 * 1024) {
        const QString oldPath = path + QStringLiteral(".old");
        QFile::remove(oldPath);
        QFile::rename(path, oldPath);
    }
}

void fileMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    static QMutex mutex;
    const QMutexLocker locker(&mutex);

    const QString formatted = qFormatLogMessage(type, context, message);
    fprintf(stderr, "%s\n", qPrintable(formatted));

    const QString path = logFilePath();
    rotateLogIfNeeded(path);

    QFile file(path);
    if (file.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&file);
        out << QDateTime::currentDateTime().toString(Qt::ISODate) << ' ' << formatted << '\n';
    }
}

} // namespace

int main(int argc, char *argv[])
{
    qInstallMessageHandler(fileMessageHandler);

    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("tegrarcm-gui"));
    QApplication::setOrganizationName(QStringLiteral("tegrarcm"));
    QApplication::setWindowIcon(QIcon(QStringLiteral(":/assets/tegrarcm.svg")));

    MainWindow window;
    window.show();

    return QApplication::exec();
}
