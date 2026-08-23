/*
 * Favorites.h
 * Part of TegraRcmGUI-Linux, a Linux port of eliboa/TegraRcmGUI.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Persistence for the favorite payload list. Stored as JSON at
 * ~/.config/tegrarcm/favorites.json (see SPEC.md section 8.2).
 */
#pragma once

#include <QObject>
#include <QStringList>

class Favorites : public QObject
{
    Q_OBJECT

public:
    explicit Favorites(QObject *parent = nullptr);

    const QStringList &paths() const { return m_paths; }
    bool contains(const QString &path) const;

    // No-op (with logMessage) if the path is already present.
    void add(const QString &path);
    void remove(const QString &path);

signals:
    void changed();
    void logMessage(const QString &message);

private:
    QString filePath() const;
    void load();
    void save() const;

    QStringList m_paths;
};
