#ifndef SHORTCUTSMANAGER_H
#define SHORTCUTSMANAGER_H

#include <QObject>
#include <QMap>
#include <QShortcut>
#include <QKeySequence>

class QWidget;

class ShortcutsManager : public QObject
{
    Q_OBJECT

public:
    explicit ShortcutsManager(QObject *parent = nullptr);

    void registerShortcut(QWidget* widget, const QString& name, const QString& sequence);
    void unregisterAll();
    void reloadFromSettings();

    QString getShortcutString(const QString& name) const;
    QKeySequence getKeySequence(const QString& name) const;

signals:
    void shortcutActivated(const QString& name);

private slots:
    void onShortcutActivated();

private:
    QMap<QString, QShortcut*> m_shortcuts;
    QMap<QString, QString> m_shortcutNames;
};

#endif
