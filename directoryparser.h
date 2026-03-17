#ifndef DIRECTORYPARSER_H
#define DIRECTORYPARSER_H

#include <QObject>
#include <QList>
#include <QString>

struct Chapter {
    QString title;
    int startLine;
    int endLine;
    int index;
};

class DirectoryParser : public QObject
{
    Q_OBJECT

public:
    explicit DirectoryParser(QObject *parent = nullptr);

    QList<Chapter> parse(const QStringList& lines);

private:
    bool isChapterTitle(const QString& line) const;
    QString extractChapterNumber(const QString& line) const;

    QStringList m_patterns;
};

#endif
