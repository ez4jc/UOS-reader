QT += core gui widgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = reader
TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS

CONFIG += c++11

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    textreader.cpp \
    directoryparser.cpp \
    settingsmanager.cpp \
    shortcutsmanager.cpp \
    systemtray.cpp \
    settingsdialog.cpp

HEADERS += \
    mainwindow.h \
    textreader.h \
    directoryparser.h \
    settingsmanager.h \
    shortcutsmanager.h \
    systemtray.h \
    settingsdialog.h

qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

desktop.path = /usr/share/applications
desktop.files = packaging/reader.desktop
!isEmpty(desktop.path): INSTALLS += desktop

RESOURCES +=
