/*
 * FavoritesTab.h
 * Part of TegraRcmGUI-Linux, a Linux port of eliboa/TegraRcmGUI.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#pragma once

#include <QWidget>

class Favorites;
class QListWidget;
class QListWidgetItem;

class FavoritesTab : public QWidget
{
    Q_OBJECT

public:
    explicit FavoritesTab(Favorites *favorites, QWidget *parent = nullptr);

signals:
    void injectRequested(const QString &path);

private slots:
    void refresh();
    void onItemDoubleClicked(QListWidgetItem *item);
    void onContextMenuRequested(const QPoint &pos);

private:
    Favorites *m_favorites;
    QListWidget *m_list;
};
