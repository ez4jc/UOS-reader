#include "systemtray.h"
#include <QWidget>
#include <QAction>
#include <QIcon>
#include <QPixmap>
#include <QPainter>

SystemTray::SystemTray(QObject *parent)
    : QObject(parent)
    , m_trayIcon(nullptr)
    , m_menu(nullptr)
    , m_parentWindow(nullptr)
{
    m_trayIcon = new QSystemTrayIcon(this);

    QPixmap pixmap(32, 32);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setPen(Qt::blue);
    painter.setFont(QFont("Arial", 16));
    painter.drawText(pixmap.rect(), Qt::AlignCenter, "R");
    QIcon icon(pixmap);
    m_trayIcon->setIcon(icon);
    m_trayIcon->setToolTip("阅读器");

    m_menu = new QMenu();

    QAction* showAction = new QAction("显示窗口", m_menu);
    connect(showAction, &QAction::triggered, this, &SystemTray::onShowWindow);
    m_menu->addAction(showAction);

    m_menu->addSeparator();

    QAction* quitAction = new QAction("退出", m_menu);
    connect(quitAction, &QAction::triggered, this, &SystemTray::onQuit);
    m_menu->addAction(quitAction);

    m_trayIcon->setContextMenu(m_menu);

    connect(m_trayIcon, &QSystemTrayIcon::activated,
            this, &SystemTray::onTrayActivated);
}

SystemTray::~SystemTray()
{
    if (m_trayIcon) {
        m_trayIcon->hide();
    }
}

void SystemTray::setParentWindow(QWidget* window)
{
    m_parentWindow = window;
}

void SystemTray::show()
{
    if (m_trayIcon) {
        m_trayIcon->show();
    }
}

void SystemTray::hide()
{
    if (m_trayIcon) {
        m_trayIcon->hide();
    }
}

void SystemTray::showMessage(const QString& title, const QString& message)
{
    if (m_trayIcon && m_trayIcon->isVisible()) {
        m_trayIcon->showMessage(title, message, QSystemTrayIcon::Information, 3000);
    }
}

void SystemTray::onTrayActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
        onShowWindow();
    }
}

void SystemTray::onShowWindow()
{
    emit requestShowWindow();
}

void SystemTray::onQuit()
{
    emit requestQuit();
}
