#include "settingsmanager.h"
#include <QCoreApplication>
#include <QDir>

SettingsManager* SettingsManager::instance()
{
    static SettingsManager instance;
    return &instance;
}

SettingsManager::SettingsManager(QObject *parent)
    : QObject(parent)
{
    QString configDir = QDir::homePath() + "/.config";
    if (!QDir(configDir).exists()) {
        QDir().mkpath(configDir);
    }
    m_configPath = configDir + "/reader.ini";

    m_settings = new QSettings(m_configPath, QSettings::IniFormat, this);

    if (m_settings->allKeys().isEmpty()) {
        initDefaults();
    }
}

SettingsManager::~SettingsManager()
{
    sync();
}

void SettingsManager::initDefaults()
{
    m_settings->beginGroup("Shortcuts");
    m_settings->setValue("ToggleTransparency", "Ctrl+Shift+T");
    m_settings->setValue("HideWindow", "Ctrl+Shift+H");
    m_settings->setValue("OpenFile", "Ctrl+O");
    m_settings->setValue("Quit", "Ctrl+Q");
    m_settings->setValue("NextChapter", "Right");
    m_settings->setValue("PrevChapter", "Left");
    m_settings->endGroup();

    m_settings->beginGroup("Appearance");
    m_settings->setValue("BackgroundOpacity", 100);
    m_settings->setValue("FontSize", 16);
    m_settings->setValue("FontColor", "#000000");
    m_settings->setValue("FontFamily", "SimSun");
    m_settings->endGroup();

    m_settings->beginGroup("File");
    m_settings->setValue("DefaultEncoding", "UTF-8");
    m_settings->setValue("AutoChapterDetection", true);
    m_settings->endGroup();

    sync();
}

QString SettingsManager::getShortcut(const QString& key) const
{
    return m_settings->value("Shortcuts/" + key).toString();
}

void SettingsManager::setShortcut(const QString& key, const QString& value)
{
    m_settings->setValue("Shortcuts/" + key, value);
    emit settingsChanged();
}

int SettingsManager::getBackgroundOpacity() const
{
    return m_settings->value("Appearance/BackgroundOpacity", 100).toInt();
}

void SettingsManager::setBackgroundOpacity(int value)
{
    m_settings->setValue("Appearance/BackgroundOpacity", value);
    emit settingsChanged();
}

int SettingsManager::getFontSize() const
{
    return m_settings->value("Appearance/FontSize", 16).toInt();
}

void SettingsManager::setFontSize(int value)
{
    m_settings->setValue("Appearance/FontSize", value);
    emit settingsChanged();
}

QString SettingsManager::getFontColor() const
{
    return m_settings->value("Appearance/FontColor", "#000000").toString();
}

void SettingsManager::setFontColor(const QString& color)
{
    m_settings->setValue("Appearance/FontColor", color);
    emit settingsChanged();
}

QString SettingsManager::getFontFamily() const
{
    return m_settings->value("Appearance/FontFamily", "SimSun").toString();
}

void SettingsManager::setFontFamily(const QString& family)
{
    m_settings->setValue("Appearance/FontFamily", family);
    emit settingsChanged();
}

QString SettingsManager::getDefaultEncoding() const
{
    return m_settings->value("File/DefaultEncoding", "UTF-8").toString();
}

void SettingsManager::setDefaultEncoding(const QString& encoding)
{
    m_settings->setValue("File/DefaultEncoding", encoding);
    emit settingsChanged();
}

bool SettingsManager::getAutoChapterDetection() const
{
    return m_settings->value("File/AutoChapterDetection", true).toBool();
}

void SettingsManager::setAutoChapterDetection(bool enabled)
{
    m_settings->setValue("File/AutoChapterDetection", enabled);
    emit settingsChanged();
}

QString SettingsManager::getLastFile() const
{
    return m_settings->value("Progress/LastFile").toString();
}

void SettingsManager::setLastFile(const QString& file)
{
    m_settings->setValue("Progress/LastFile", file);
}

int SettingsManager::getLastChapter() const
{
    return m_settings->value("Progress/LastChapter", 0).toInt();
}

void SettingsManager::setLastChapter(int chapter)
{
    m_settings->setValue("Progress/LastChapter", chapter);
}

int SettingsManager::getLastScrollPos() const
{
    return m_settings->value("Progress/LastScrollPos", 0).toInt();
}

void SettingsManager::setLastScrollPos(int pos)
{
    m_settings->setValue("Progress/LastScrollPos", pos);
}

void SettingsManager::sync()
{
    m_settings->sync();
}
