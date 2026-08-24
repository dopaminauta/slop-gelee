/*
 * MainWindow.cpp
 * Part of TegraRcmGUI-Linux, a Linux port of eliboa/TegraRcmGUI.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "MainWindow.h"
#include "Favorites.h"
#include "FavoritesTab.h"
#include "Injector.h"
#include "SettingsTab.h"
#include "ToolsTab.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSettings>
#include <QStatusBar>
#include <QStyle>
#include <QTabWidget>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_injector(new Injector(this))
    , m_favorites(new Favorites(this))
    , m_tabs(new QTabWidget(this))
    , m_favoritesTab(nullptr)
    , m_deviceStatusLabel(new QLabel(tr("Waiting for device..."), this))
    , m_payloadPathEdit(nullptr)
    , m_injectButton(nullptr)
    , m_saveAsFavoriteButton(nullptr)
    , m_trayIcon(nullptr)
{
    setWindowTitle(tr("slop gelee"));
    resize(640, 480);

    m_tabs->addTab(makePayloadTab(), tr("main"));
    m_tabs->addTab(new ToolsTab(m_injector, m_tabs), tr("Tools"));
    m_tabs->addTab(new SettingsTab(m_tabs), tr("Settings"));
    setCentralWidget(m_tabs);

    statusBar()->addWidget(m_deviceStatusLabel);

    connect(m_injector, &Injector::deviceConnected, this, &MainWindow::onDeviceConnected);
    connect(m_injector, &Injector::deviceDisconnected, this, &MainWindow::onDeviceDisconnected);
    connect(m_injector, &Injector::logMessage, this, &MainWindow::onLogMessage);
    connect(m_injector, &Injector::payloadInjected, this, &MainWindow::onPayloadInjected);
    connect(m_injector, &Injector::injectionFailed, this, &MainWindow::onInjectionFailed);
    connect(m_favorites, &Favorites::logMessage, this, &MainWindow::onLogMessage);
    connect(m_favoritesTab, &FavoritesTab::injectRequested, this, &MainWindow::injectPath);

    setupTrayIcon();

    m_injector->startMonitoring();
}

MainWindow::~MainWindow() = default;

QWidget *MainWindow::makePayloadTab()
{
    auto *widget = new QWidget(m_tabs);
    auto *layout = new QVBoxLayout(widget);

    auto *selectRow = new QHBoxLayout;
    auto *selectButton = new QPushButton(tr("Select payload..."), widget);
    m_payloadPathEdit = new QLineEdit(widget);
    m_payloadPathEdit->setReadOnly(true);
    m_payloadPathEdit->setPlaceholderText(tr("No payload selected"));
    // Pre-cargar el payload por defecto (release instalado o primer favorito)
    const QString defPayload = defaultPayloadPath();
    if (!defPayload.isEmpty()) {
        m_payloadPathEdit->setText(defPayload);
    }
    selectRow->addWidget(selectButton);
    selectRow->addWidget(m_payloadPathEdit, 1);

    m_injectButton = new QPushButton(tr("Inject"), widget);
    m_injectButton->setEnabled(false);

    m_saveAsFavoriteButton = new QPushButton(tr("Save as favorite"), widget);
    m_saveAsFavoriteButton->setEnabled(false);

    layout->addLayout(selectRow);
    layout->addWidget(m_injectButton);
    layout->addWidget(m_saveAsFavoriteButton);

    // Favorites integrated into the main tab (double-click to inject).
    auto *favLabel = new QLabel(tr("Favorites (double-click to inject):"), widget);
    m_favoritesTab = new FavoritesTab(m_favorites, widget);
    layout->addWidget(favLabel);
    layout->addWidget(m_favoritesTab, 1);  // stretch 1: ocupa el espacio vertical libre

    layout->addStretch();

    connect(selectButton, &QPushButton::clicked, this, &MainWindow::onSelectPayload);
    connect(m_injectButton, &QPushButton::clicked, this, &MainWindow::onInjectClicked);
    connect(m_saveAsFavoriteButton, &QPushButton::clicked,
            this, &MainWindow::onSaveAsFavoriteClicked);

    return widget;
}

void MainWindow::setupTrayIcon()
{
    m_trayIcon = new QSystemTrayIcon(QIcon(QStringLiteral(":/assets/tegrarcm.svg")), this);
    m_trayIcon->setToolTip(tr("slop gelee — Fusee Gelee payload injector"));

    auto *menu = new QMenu(this);
    QAction *showHideAction = menu->addAction(tr("Show/Hide"));
    connect(showHideAction, &QAction::triggered, this, [this]() {
        setVisible(!isVisible());
        if (isVisible()) {
            raise();
            activateWindow();
        }
    });

    menu->addSeparator();
    QAction *quitAction = menu->addAction(tr("Quit"));
    connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);

    m_trayIcon->setContextMenu(menu);
    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::onTrayActivated);

    m_trayIcon->show();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    QSettings settings;
    const bool minimizeToTray = settings.value(SettingsTab::kMinimizeToTrayKey, false).toBool();
    if (minimizeToTray && m_trayIcon->isVisible()) {
        hide();
        event->ignore();
        return;
    }
    event->accept();
}

void MainWindow::onTrayActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger) {
        setVisible(!isVisible());
        if (isVisible()) {
            raise();
            activateWindow();
        }
    }
}

void MainWindow::onSelectPayload()
{
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Select payload"), QString(), tr("Bin files (*.bin);;All files (*)"));
    if (path.isEmpty()) {
        return;
    }
    m_payloadPathEdit->setText(path);
    updateInjectEnabled();
}

void MainWindow::onInjectClicked()
{
    injectPath(m_payloadPathEdit->text());
}

void MainWindow::onSaveAsFavoriteClicked()
{
    const QString path = m_payloadPathEdit->text();
    if (path.isEmpty()) {
        return;
    }
    m_favorites->add(path);
}

void MainWindow::injectPath(const QString &path)
{
    if (path.isEmpty()) {
        return;
    }
    m_injectButton->setEnabled(false);
    m_deviceStatusLabel->setText(tr("Injecting..."));
    m_injector->injectPayload(path);
}

QString MainWindow::defaultPayloadPath() const
{
    // 1) El payload que el usuario selecciono
    if (!m_payloadPathEdit->text().isEmpty()) {
        return m_payloadPathEdit->text();
    }
    // 2) El payload del release instalado (install.sh lo deja aca)
    const QDir payloadDir(QDir::homePath() + QStringLiteral("/.local/share/tegrarcm/payloads"));
    const QStringList bins =
        payloadDir.entryList({QStringLiteral("*.bin")}, QDir::Files, QDir::Name);
    if (!bins.isEmpty()) {
        return payloadDir.filePath(bins.first());
    }
    // 3) Primer favorito
    if (m_favorites != nullptr && !m_favorites->paths().isEmpty()) {
        return m_favorites->paths().first();
    }
    return {};
}

void MainWindow::updateInjectEnabled()
{
    const bool hasPath = !m_payloadPathEdit->text().isEmpty();
    m_injectButton->setEnabled(hasPath && m_injector->isDeviceConnected());
    m_saveAsFavoriteButton->setEnabled(hasPath);
}

void MainWindow::onDeviceConnected()
{
    m_deviceStatusLabel->setText(tr("Device found (RCM)"));
    updateInjectEnabled();

    // Auto-injection: OPTIONAL, default OFF (user product decision).
    QSettings settings;
    const bool autoInject = settings.value(SettingsTab::kAutoInjectKey, false).toBool();
    const QString path = defaultPayloadPath();
    if (autoInject && !path.isEmpty()) {
        injectPath(path);
    }
}

void MainWindow::onDeviceDisconnected()
{
    m_deviceStatusLabel->setText(tr("Waiting for device..."));
    updateInjectEnabled();
}

void MainWindow::onPayloadInjected(const QString &path, int bytesSent)
{
    Q_UNUSED(bytesSent)
    Q_UNUSED(path)
    m_deviceStatusLabel->setText(tr("Payload injected"));
    updateInjectEnabled();
}

void MainWindow::onInjectionFailed(const QString &errorMessage)
{
    m_deviceStatusLabel->setText(tr("Injection failed"));
    statusBar()->showMessage(errorMessage, 8000);
    // Persist errors too (non-zero exits from the engine do not go through onLogMessage)
    onLogMessage(QStringLiteral("ERROR: %1").arg(errorMessage));
    updateInjectEnabled();
}

void MainWindow::onLogMessage(const QString &message)
{
    qDebug() << message;
    // Log persistente para diagnostico (reversible; ayuda a ver el output del
    // injection engine without depending on the window).
    QDir logDir(QDir::homePath() + QStringLiteral("/.local/share/tegrarcm"));
    logDir.mkpath(QStringLiteral("."));
    QFile logFile(logDir.filePath(QStringLiteral("log.txt")));
    if (logFile.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&logFile);
        out << QDateTime::currentDateTime().toString(Qt::ISODate) << " " << message << Qt::endl;
    }
}
