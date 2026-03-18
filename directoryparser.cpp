#include "directoryparser.h"
#include <QRegularExpression>

DirectoryParser::DirectoryParser(QObject *parent)
    : QObject(parent)
{
    m_patterns << "^第[一二三四五六七八九十百千0-9零一二三四五六七八九十百千]+章.*"
               << "^第[一二三四五六七八九十百千0-9零一二三四五六七八九十百千]+篇.*"
               << "^第[一二三四五六七八九十百千0-9零一二三四五六七八九十百千]+部.*"
               << "^Chapter\\s+\\d+.*"
               << "^CHAPTER\\s+\\d+.*"
               << "^Part\\s+[IVXLC]+.*"
               << "^第[0-9]+节.*"
               << "^卷[0-9一二三四五六七八九十]+.*"
               << "^Volume\\s+\\d+.*"
               << "^Vol\\.[\\s\\S]+";
}

QList<Chapter> DirectoryParser::parse(const QStringList& lines)
{
    QList<Chapter> chapters;
    int chapterIndex = 0;

    for (int i = 0; i < lines.size(); ++i) {
        const QString& line = lines.at(i).trimmed();

        if (isChapterTitle(line)) {
            if (chapterIndex > 0) {
                chapters[chapterIndex - 1].endLine = i - 1;
            }

            Chapter ch;
            ch.title = line;
            ch.startLine = i;
            ch.index = chapterIndex;
            chapters.append(ch);

            ++chapterIndex;
        }
    }

    if (!chapters.isEmpty()) {
        chapters.last().endLine = lines.size() - 1;
    }

    if (chapters.isEmpty() && !lines.isEmpty()) {
        Chapter singleChapter;
        singleChapter.title = "全文";
        singleChapter.startLine = 0;
        singleChapter.endLine = lines.size() - 1;
        singleChapter.index = 0;
        chapters.append(singleChapter);
    }

    return chapters;
}

bool DirectoryParser::isChapterTitle(const QString& line) const
{
    if (line.length() > 100 || line.length() < 2) {
        return false;
    }

    if (isLikelyNumericHeading(line)) {
        return true;
    }

    for (const QString& pattern : m_patterns) {
        QRegularExpression regex(pattern);
        if (regex.match(line).hasMatch()) {
            return true;
        }
    }

    return false;
}

bool DirectoryParser::isLikelyNumericHeading(const QString& line) const
{
    const QString trimmed = line.trimmed();
    if (trimmed.length() < 2 || trimmed.length() > 40) {
        return false;
    }

    static const QRegularExpression numericHeadingRegex("^\\d{1,4}[、．.](?!\\d)\\s*\\S.*$");
    if (!numericHeadingRegex.match(trimmed).hasMatch()) {
        return false;
    }

    static const QRegularExpression sentencePunctuationRegex("[，。！？；：,!?;:]");
    if (sentencePunctuationRegex.match(trimmed).hasMatch()) {
        return false;
    }

    return true;
}

QString DirectoryParser::extractChapterNumber(const QString& line) const
{
    QRegularExpression numExpr("[0-9]+");
    QRegularExpressionMatch match = numExpr.match(line);
    if (match.hasMatch()) {
        return match.captured(0);
    }
    return QString();
}
