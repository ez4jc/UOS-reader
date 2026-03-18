#include "globalhotkey.h"

#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusReply>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeySequence>
#include <QProcessEnvironment>
#include <QSocketNotifier>
#include <QStandardPaths>

#include <X11/Xlib.h>
#include <X11/keysym.h>

namespace {

const char kDeepinService[] = "com.deepin.daemon.Keybinding";
const char kDeepinPath[] = "/com/deepin/daemon/Keybinding";
const char kDeepinInterface[] = "com.deepin.daemon.Keybinding";
const char kDeepinShortcutName[] = "ReaderToggleVisibility";

unsigned long keySymFromPortableText(const QString& text)
{
    const QByteArray latin = text.toLatin1();
    if (latin.isEmpty()) {
        return NoSymbol;
    }

    return XStringToKeysym(latin.constData());
}

}

GlobalHotkey::GlobalHotkey(QObject *parent)
    : QObject(parent)
    , m_backend(NoBackend)
    , m_display(nullptr)
    , m_notifier(nullptr)
    , m_activationCommand()
    , m_deepinShortcutId()
    , m_keycode(0)
    , m_modifiers(0)
    , m_numLockMask(0)
{
    const QString sessionType = QProcessEnvironment::systemEnvironment().value("XDG_SESSION_TYPE").toLower();
    const QString currentDesktop = QProcessEnvironment::systemEnvironment().value("XDG_CURRENT_DESKTOP").toLower();

    if (sessionType == "wayland" && currentDesktop.contains("deepin")) {
        QDBusInterface iface(kDeepinService, kDeepinPath, kDeepinInterface, QDBusConnection::sessionBus());
        if (iface.isValid()) {
            m_backend = DeepinBackend;
            return;
        }
    }

    if (sessionType != "wayland") {
        m_display = XOpenDisplay(nullptr);
        if (!m_display) {
            return;
        }

        m_backend = X11Backend;
        detectNumLockMask();

        m_notifier = new QSocketNotifier(ConnectionNumber(m_display), QSocketNotifier::Read, this);
        connect(m_notifier, &QSocketNotifier::activated, this, &GlobalHotkey::processX11Events);
    }
}

GlobalHotkey::~GlobalHotkey()
{
    unregisterCurrentShortcut();

    if (m_display) {
        XCloseDisplay(m_display);
    }
}

bool GlobalHotkey::isAvailable() const
{
    return m_backend != NoBackend;
}

void GlobalHotkey::setActivationCommand(const QString& command)
{
    m_activationCommand = command.trimmed();
}

bool GlobalHotkey::setShortcut(const QString& sequence)
{
    unregisterCurrentShortcut();
    m_sequence = sequence.trimmed();

    if (m_sequence.isEmpty()) {
        return true;
    }

    if (m_backend == NoBackend) {
        return false;
    }

    return registerCurrentShortcut();
}

QString GlobalHotkey::shortcut() const
{
    return m_sequence;
}

void GlobalHotkey::processX11Events()
{
    if (m_backend != X11Backend || !m_display) {
        return;
    }

    while (XPending(m_display) > 0) {
        XEvent event;
        XNextEvent(m_display, &event);

        if (event.type != KeyPress || m_keycode == 0) {
            continue;
        }

        const unsigned int state = currentModifiersWithLocksCleared(event.xkey.state);
        if (static_cast<unsigned int>(event.xkey.keycode) == m_keycode && state == m_modifiers) {
            emit activated();
        }
    }
}

bool GlobalHotkey::registerCurrentShortcut()
{
    if (m_backend == DeepinBackend) {
        return registerDeepinShortcut();
    }

    if (m_backend != X11Backend || !m_display) {
        return false;
    }

    const QKeySequence keySequence(m_sequence, QKeySequence::PortableText);
    if (keySequence.count() != 1 || keySequence[0] == 0) {
        return false;
    }

    m_keycode = keycodeForSequence(keySequence[0]);
    m_modifiers = modifierMaskForSequence(keySequence[0]);

    if (m_keycode == 0) {
        return false;
    }

    const Window rootWindow = DefaultRootWindow(m_display);
    const unsigned int masks[] = {
        m_modifiers,
        m_modifiers | LockMask,
        m_modifiers | m_numLockMask,
        m_modifiers | LockMask | m_numLockMask
    };

    for (unsigned int mask : masks) {
        XGrabKey(m_display, static_cast<int>(m_keycode), mask, rootWindow, True, GrabModeAsync, GrabModeAsync);
    }

    XSync(m_display, False);
    return true;
}

void GlobalHotkey::unregisterCurrentShortcut()
{
    if (m_backend == DeepinBackend) {
        unregisterDeepinShortcut();
        return;
    }

    if (!m_display || m_keycode == 0) {
        m_keycode = 0;
        m_modifiers = 0;
        return;
    }

    const Window rootWindow = DefaultRootWindow(m_display);
    const unsigned int masks[] = {
        m_modifiers,
        m_modifiers | LockMask,
        m_modifiers | m_numLockMask,
        m_modifiers | LockMask | m_numLockMask
    };

    for (unsigned int mask : masks) {
        XUngrabKey(m_display, static_cast<int>(m_keycode), mask, rootWindow);
    }

    XSync(m_display, False);
    m_keycode = 0;
    m_modifiers = 0;
}

bool GlobalHotkey::registerDeepinShortcut()
{
    if (m_backend != DeepinBackend || m_activationCommand.isEmpty()) {
        return false;
    }

    const QString accel = toDeepinAccelerator(m_sequence);
    if (accel.isEmpty()) {
        return false;
    }

    QDBusInterface iface(kDeepinService, kDeepinPath, kDeepinInterface, QDBusConnection::sessionBus());
    if (!iface.isValid()) {
        return false;
    }

    QString shortcutId = findDeepinShortcutId();
    if (shortcutId.isEmpty()) {
        QDBusMessage reply = iface.call("AddCustomShortcut",
                                        QString::fromLatin1(kDeepinShortcutName),
                                        m_activationCommand,
                                        accel);
        if (reply.type() == QDBusMessage::ErrorMessage || reply.arguments().isEmpty()) {
            return false;
        }
        shortcutId = reply.arguments().at(0).toString();
    } else {
        QDBusMessage reply = iface.call("ModifyCustomShortcut",
                                        shortcutId,
                                        QString::fromLatin1(kDeepinShortcutName),
                                        m_activationCommand,
                                        accel);
        if (reply.type() == QDBusMessage::ErrorMessage) {
            return false;
        }
    }

    m_deepinShortcutId = shortcutId;
    return !m_deepinShortcutId.isEmpty();
}

void GlobalHotkey::unregisterDeepinShortcut()
{
    if (m_backend != DeepinBackend) {
        return;
    }

    QString shortcutId = m_deepinShortcutId;
    if (shortcutId.isEmpty()) {
        shortcutId = findDeepinShortcutId();
    }
    if (shortcutId.isEmpty()) {
        return;
    }

    QDBusInterface iface(kDeepinService, kDeepinPath, kDeepinInterface, QDBusConnection::sessionBus());
    if (!iface.isValid()) {
        return;
    }

    iface.call("DeleteCustomShortcut", shortcutId);
    m_deepinShortcutId.clear();
}

QString GlobalHotkey::findDeepinShortcutId() const
{
    QDBusInterface iface(kDeepinService, kDeepinPath, kDeepinInterface, QDBusConnection::sessionBus());
    if (!iface.isValid()) {
        return QString();
    }

    QDBusReply<QString> reply = iface.call("ListShortcutsByType", 1);
    if (!reply.isValid()) {
        return QString();
    }

    const QJsonDocument jsonDoc = QJsonDocument::fromJson(reply.value().toUtf8());
    if (!jsonDoc.isArray()) {
        return QString();
    }

    const QJsonArray shortcuts = jsonDoc.array();
    for (const QJsonValue& shortcutValue : shortcuts) {
        const QJsonObject shortcutObject = shortcutValue.toObject();
        if (shortcutObject.value("Name").toString() == QString::fromLatin1(kDeepinShortcutName) ||
            shortcutObject.value("Exec").toString() == m_activationCommand) {
            return shortcutObject.value("Id").toString();
        }
    }

    return QString();
}

QString GlobalHotkey::toDeepinAccelerator(const QString& sequence) const
{
    const QKeySequence keySequence(sequence, QKeySequence::PortableText);
    if (keySequence.count() != 1 || keySequence[0] == 0) {
        return QString();
    }

    QString portable = keySequence.toString(QKeySequence::PortableText);
    portable.replace("Ctrl+", "<Control>");
    portable.replace("Alt+", "<Alt>");
    portable.replace("Shift+", "<Shift>");
    portable.replace("Meta+", "<Super>");
    return portable;
}

unsigned int GlobalHotkey::currentModifiersWithLocksCleared(unsigned int state) const
{
    return state & ~(LockMask | m_numLockMask);
}

unsigned int GlobalHotkey::modifierMaskForSequence(int sequence) const
{
    const Qt::KeyboardModifiers modifiers = Qt::KeyboardModifiers(sequence & Qt::KeyboardModifierMask);

    unsigned int mask = 0;
    if (modifiers & Qt::ShiftModifier) {
        mask |= ShiftMask;
    }
    if (modifiers & Qt::ControlModifier) {
        mask |= ControlMask;
    }
    if (modifiers & Qt::AltModifier) {
        mask |= Mod1Mask;
    }
    if (modifiers & Qt::MetaModifier) {
        mask |= Mod4Mask;
    }

    return mask;
}

unsigned int GlobalHotkey::keycodeForSequence(int sequence) const
{
    if (!m_display) {
        return 0;
    }

    const int qtKey = sequence & ~Qt::KeyboardModifierMask;
    const unsigned long keySym = keySymForQtKey(qtKey);
    if (keySym == NoSymbol) {
        return 0;
    }

    return XKeysymToKeycode(m_display, keySym);
}

unsigned long GlobalHotkey::keySymForQtKey(int qtKey) const
{
    switch (qtKey) {
    case Qt::Key_Left:
        return XK_Left;
    case Qt::Key_Right:
        return XK_Right;
    case Qt::Key_Up:
        return XK_Up;
    case Qt::Key_Down:
        return XK_Down;
    case Qt::Key_PageUp:
        return XK_Page_Up;
    case Qt::Key_PageDown:
        return XK_Page_Down;
    case Qt::Key_Home:
        return XK_Home;
    case Qt::Key_End:
        return XK_End;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        return XK_Return;
    case Qt::Key_Space:
        return XK_space;
    case Qt::Key_Tab:
        return XK_Tab;
    case Qt::Key_Backtab:
        return XK_ISO_Left_Tab;
    case Qt::Key_Escape:
        return XK_Escape;
    case Qt::Key_Delete:
        return XK_Delete;
    case Qt::Key_Backspace:
        return XK_BackSpace;
    default:
        break;
    }

    if (qtKey >= Qt::Key_F1 && qtKey <= Qt::Key_F35) {
        return XK_F1 + (qtKey - Qt::Key_F1);
    }

    const QString portableText = QKeySequence(qtKey).toString(QKeySequence::PortableText);
    unsigned long keySym = keySymFromPortableText(portableText);
    if (keySym != NoSymbol) {
        return keySym;
    }

    if (portableText.size() == 1) {
        return keySymFromPortableText(portableText.toUpper());
    }

    return NoSymbol;
}

void GlobalHotkey::detectNumLockMask()
{
    if (!m_display) {
        return;
    }

    XModifierKeymap* modifierMap = XGetModifierMapping(m_display);
    if (!modifierMap) {
        return;
    }

    const KeyCode numLockKeycode = XKeysymToKeycode(m_display, XK_Num_Lock);
    for (int modifierIndex = 0; modifierIndex < 8; ++modifierIndex) {
        for (int keyIndex = 0; keyIndex < modifierMap->max_keypermod; ++keyIndex) {
            const int offset = modifierIndex * modifierMap->max_keypermod + keyIndex;
            if (modifierMap->modifiermap[offset] == numLockKeycode) {
                m_numLockMask = (1u << modifierIndex);
                break;
            }
        }

        if (m_numLockMask != 0) {
            break;
        }
    }

    XFreeModifiermap(modifierMap);
}
