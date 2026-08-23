/*
 * Favorites.cpp
 * Part of TegraRcmGUI-Linux, a Linux port of eliboa/TegraRcmGUI.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "Favorites.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

Favorites::Favorites(QObject *parent)
    : QObject(parent)
{
    load();
}

bool Favorites::contains(const QString &path) const
{
    return m_paths.contains(path);
}

void Favorites::add(const QString &path)
{
    if (contains(path)) {
        return;
    }
    m_paths.append(path);
    save();
    emit changed();
}

void Favorites::remove(const QString &path)
{
    if (m_paths.removeAll(path) == 0) {
        return;
    }
    save();
    emit changed();
}

QString Favorites::filePath() const
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
        + QStringLiteral("/tegrarcm");
    return dir + QStringLiteral("/favorites.json");
}

void Favorites::load()
{
    QFile file(filePath());
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) {
        emit logMessage(QStringLiteral("favorites.json invalido, se ignora"));
        return;
    }

    const QJsonArray array = doc.object().value(QStringLiteral("favorites")).toArray();
    for (const QJsonValue &value : array) {
        if (value.isString()) {
            m_paths.append(value.toString());
        }
    }
}

void Favorites::save() const
{
    const QString path = filePath();
    QDir().mkpath(QFileInfo(path).absolutePath());

    QJsonArray array;
    for (const QString &favorite : m_paths) {
        array.append(favorite);
    }

    QJsonObject root;
    root.insert(QStringLiteral("favorites"), array);

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return;
    }
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
}
