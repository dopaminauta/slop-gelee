/*
 * MainWindow.h
 * Part of TegraRcmGUI-Linux, a Linux port of eliboa/TegraRcmGUI.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#pragma once

#include <QMainWindow>
#include <QSystemTrayIcon>

class Favorites;
class FavoritesTab;
class Injector;
class QCloseEvent;
class QLabel;
class QLineEdit;
class QPushButton;
class QTabWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onDeviceConnected();
    void onDeviceDisconnected();
    void onLogMessage(const QString &message);
    void onPayloadInjected(const QString &path, int bytesSent);
    void onInjectionFailed(const QString &errorMessage);
    void onSelectPayload();
    void onInjectClicked();
    void onSaveAsFavoriteClicked();
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);

private:
    QWidget *makePayloadTab();
    void injectPath(const QString &path);
    QString defaultPayloadPath() const;
    void updateInjectEnabled();
    void setupTrayIcon();

    Injector *m_injector;
    Favorites *m_favorites;
    QTabWidget *m_tabs;
    FavoritesTab *m_favoritesTab;
    QLabel *m_deviceStatusLabel;
    QLineEdit *m_payloadPathEdit;
    QPushButton *m_injectButton;
    QPushButton *m_saveAsFavoriteButton;
    QSystemTrayIcon *m_trayIcon;
};
