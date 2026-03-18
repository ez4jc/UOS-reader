#ifndef GLOBALHOTKEY_H
#define GLOBALHOTKEY_H

#include <QObject>

class QSocketNotifier;
struct _XDisplay;

class GlobalHotkey : public QObject
{
    Q_OBJECT

public:
    explicit GlobalHotkey(QObject *parent = nullptr);
    ~GlobalHotkey();

    bool isAvailable() const;
    void setActivationCommand(const QString& command);
    bool setShortcut(const QString& sequence);
    QString shortcut() const;

signals:
    void activated();

private slots:
    void processX11Events();

private:
    enum Backend {
        NoBackend,
        X11Backend,
        DeepinBackend
    };

    bool registerCurrentShortcut();
    void unregisterCurrentShortcut();
    bool registerDeepinShortcut();
    void unregisterDeepinShortcut();
    QString findDeepinShortcutId() const;
    QString toDeepinAccelerator(const QString& sequence) const;
    unsigned int currentModifiersWithLocksCleared(unsigned int state) const;
    unsigned int modifierMaskForSequence(int sequence) const;
    unsigned int keycodeForSequence(int sequence) const;
    unsigned long keySymForQtKey(int qtKey) const;
    void detectNumLockMask();

    Backend m_backend;
    _XDisplay* m_display;
    QSocketNotifier* m_notifier;
    QString m_sequence;
    QString m_activationCommand;
    QString m_deepinShortcutId;
    unsigned int m_keycode;
    unsigned int m_modifiers;
    unsigned int m_numLockMask;
};

#endif
