#ifndef TEXTREADER_H
#define TEXTREADER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QList>
#include "directoryparser.h"

class TextReader : public QObject
{
    Q_OBJECT

public:
    explicit TextReader(QObject *parent = nullptr);

    bool loadFile(const QString& filePath);
    QString getContent() const;
    QString getChapterContent(int chapterIndex) const;
    QList<Chapter> getChapters() const;
    QString getFileName() const;
    QString getEncoding() const;
    bool isLoaded() const;

signals:
    void fileLoaded(const QString& filePath);
    void loadError(const QString& error);

private:
    QString detectEncoding(const QString& filePath);
    QString readFileContent(const QString& filePath, const QString& encoding);

    QString m_filePath;
    QString m_fileName;
    QString m_encoding;
    QString m_content;
    QStringList m_lines;
    QList<Chapter> m_chapters;
    bool m_loaded;
    DirectoryParser* m_parser;
};

#endif
