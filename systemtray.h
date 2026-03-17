#ifndef SYSTEMTRAY_H
#define SYSTEMTRAY_H

#include <QObject>
#include <QSystemTrayIcon>
#include <QMenu>

class QWidget;

class SystemTray : public QObject
{
    Q_OBJECT

public:
    explicit SystemTray(QObject *parent = nullptr);
    ~SystemTray();

    void setParentWindow(QWidget* window);
    void show();
    void hide();
    void showMessage(const QString& title, const QString& message);

signals:
    void requestShowWindow();
    void requestQuit();

private slots:
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);
    void onShowWindow();
    void onQuit();

private:
    QSystemTrayIcon* m_trayIcon;
    QMenu* m_menu;
    QWidget* m_parentWindow;
};

#endif
