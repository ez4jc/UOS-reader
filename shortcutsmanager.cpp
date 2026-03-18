#include "shortcutsmanager.h"
#include "settingsmanager.h"
#include <QWidget>
#include <QDebug>

ShortcutsManager::ShortcutsManager(QObject *parent)
    : QObject(parent)
{
}

void ShortcutsManager::registerShortcut(QWidget* widget, const QString& name, const QString& sequence)
{
    if (m_shortcuts.contains(name)) {
        QShortcut* oldShortcut = m_shortcuts.value(name);
        oldShortcut->disconnect();
        oldShortcut->deleteLater();
        m_shortcuts.remove(name);
    }

    m_shortcutNames.insert(name, sequence);
    if (sequence.trimmed().isEmpty()) {
        return;
    }

    QKeySequence keySeq(sequence);
    QShortcut* shortcut = new QShortcut(keySeq, widget);
    shortcut->setContext(Qt::ApplicationShortcut);
    connect(shortcut, &QShortcut::activated, this, &ShortcutsManager::onShortcutActivated);

    m_shortcuts.insert(name, shortcut);
}

void ShortcutsManager::unregisterAll()
{
    QMapIterator<QString, QShortcut*> it(m_shortcuts);
    while (it.hasNext()) {
        it.next();
        it.value()->deleteLater();
    }
    m_shortcuts.clear();
    m_shortcutNames.clear();
}

void ShortcutsManager::reloadFromSettings()
{
    SettingsManager* settings = SettingsManager::instance();

    const QStringList shortcutNames = m_shortcutNames.keys();
    for (const QString& name : shortcutNames) {
        const QString newSequence = settings->getShortcut(name);
        if (m_shortcuts.contains(name)) {
            QShortcut* shortcut = m_shortcuts.value(name);
            shortcut->setKey(QKeySequence(newSequence));
        }
        m_shortcutNames.insert(name, newSequence);
    }
}

QString ShortcutsManager::getShortcutString(const QString& name) const
{
    return m_shortcutNames.value(name);
}

QKeySequence ShortcutsManager::getKeySequence(const QString& name) const
{
    return QKeySequence(m_shortcutNames.value(name));
}

void ShortcutsManager::onShortcutActivated()
{
    QShortcut* shortcut = qobject_cast<QShortcut*>(sender());
    if (shortcut) {
        QMapIterator<QString, QShortcut*> it(m_shortcuts);
        while (it.hasNext()) {
            it.next();
            if (it.value() == shortcut) {
                emit shortcutActivated(it.key());
                break;
            }
        }
    }
}
