/*
 * FavoritesTab.cpp
 * Part of TegraRcmGUI-Linux, a Linux port of eliboa/TegraRcmGUI.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "FavoritesTab.h"
#include "Favorites.h"

#include <QAction>
#include <QListWidget>
#include <QMenu>
#include <QVBoxLayout>

FavoritesTab::FavoritesTab(Favorites *favorites, QWidget *parent)
    : QWidget(parent)
    , m_favorites(favorites)
    , m_list(new QListWidget(this))
{
    auto *layout = new QVBoxLayout(this);
    m_list->setContextMenuPolicy(Qt::CustomContextMenu);
    layout->addWidget(m_list);

    connect(m_list, &QListWidget::itemDoubleClicked, this, &FavoritesTab::onItemDoubleClicked);
    connect(m_list, &QListWidget::customContextMenuRequested,
            this, &FavoritesTab::onContextMenuRequested);
    connect(m_favorites, &Favorites::changed, this, &FavoritesTab::refresh);

    refresh();
}

void FavoritesTab::refresh()
{
    m_list->clear();
    m_list->addItems(m_favorites->paths());
}

void FavoritesTab::onItemDoubleClicked(QListWidgetItem *item)
{
    if (item) {
        emit injectRequested(item->text());
    }
}

void FavoritesTab::onContextMenuRequested(const QPoint &pos)
{
    QListWidgetItem *item = m_list->itemAt(pos);
    if (!item) {
        return;
    }

    QMenu menu(this);
    QAction *sendAction = menu.addAction(tr("send"));
    QAction *removeAction = menu.addAction(tr("remove"));

    QAction *chosen = menu.exec(m_list->mapToGlobal(pos));
    if (chosen == sendAction) {
        emit injectRequested(item->text());
    } else if (chosen == removeAction) {
        m_favorites->remove(item->text());
    }
}
