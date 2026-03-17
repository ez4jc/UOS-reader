#ifndef SETTINGSMANAGER_H
#define SETTINGSMANAGER_H

#include <QObject>
#include <QSettings>
#include <QString>

class SettingsManager : public QObject
{
    Q_OBJECT

public:
    static SettingsManager* instance();

    QString getShortcut(const QString& key) const;
    void setShortcut(const QString& key, const QString& value);

    int getBackgroundOpacity() const;
    void setBackgroundOpacity(int value);

    int getFontSize() const;
    void setFontSize(int value);

    QString getFontColor() const;
    void setFontColor(const QString& color);

    QString getFontFamily() const;
    void setFontFamily(const QString& family);

    QString getDefaultEncoding() const;
    void setDefaultEncoding(const QString& encoding);

    bool getAutoChapterDetection() const;
    void setAutoChapterDetection(bool enabled);

    void sync();

signals:
    void settingsChanged();

private:
    explicit SettingsManager(QObject *parent = nullptr);
    ~SettingsManager();

    QSettings* m_settings;
    QString m_configPath;

    void initDefaults();
};

#endif
