#include "textreader.h"
#include <QFile>
#include <QFileInfo>
#include <QTextCodec>
#include <QDebug>

TextReader::TextReader(QObject *parent)
    : QObject(parent)
    , m_loaded(false)
    , m_parser(new DirectoryParser(this))
{
}

bool TextReader::loadFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.exists()) {
        emit loadError("文件不存在: " + filePath);
        return false;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        emit loadError("无法打开文件: " + file.errorString());
        return false;
    }

    m_encoding = detectEncoding(filePath);
    file.close();

    m_content = readFileContent(filePath, m_encoding);
    if (m_content.isEmpty()) {
        emit loadError("文件为空或读取失败");
        return false;
    }

    m_lines = m_content.split('\n');
    m_chapters = m_parser->parse(m_lines);

    QFileInfo fileInfo(filePath);
    m_fileName = fileInfo.fileName();
    m_filePath = filePath;
    m_loaded = true;

    emit fileLoaded(m_filePath);
    return true;
}

QString TextReader::getContent() const
{
    return m_content;
}

QString TextReader::getChapterContent(int chapterIndex) const
{
    if (chapterIndex < 0 || chapterIndex >= m_chapters.size()) {
        return QString();
    }

    const Chapter& chapter = m_chapters.at(chapterIndex);
    QStringList chapterLines;

    for (int i = chapter.startLine; i <= chapter.endLine && i < m_lines.size(); ++i) {
        chapterLines.append(m_lines.at(i));
    }

    return chapterLines.join('\n');
}

QList<Chapter> TextReader::getChapters() const
{
    return m_chapters;
}

QString TextReader::getFileName() const
{
    return m_fileName;
}

QString TextReader::getEncoding() const
{
    return m_encoding;
}

bool TextReader::isLoaded() const
{
    return m_loaded;
}

QString TextReader::detectEncoding(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return "UTF-8";
    }

    QByteArray data = file.read(4096);
    file.close();

    if (data.startsWith("\xEF\xBB\xBF")) {
        return "UTF-8";
    }
    if (data.startsWith("\xFF\xFE") || data.startsWith("\xFE\xFF")) {
        return "UTF-16";
    }

    bool isAsciiOnly = true;
    for (int i = 0; i < data.size() && i < 4096; ++i) {
        if (static_cast<unsigned char>(data[i]) > 127) {
            isAsciiOnly = false;
            break;
        }
    }

    if (isAsciiOnly) {
        return "UTF-8";
    }

    QTextCodec* utf8Codec = QTextCodec::codecForName("UTF-8");
    if (utf8Codec) {
        QString decoded = utf8Codec->toUnicode(data);
        QByteArray reEncoded = utf8Codec->fromUnicode(decoded);
        if (reEncoded == data) {
            return "UTF-8";
        }
    }

    return "GBK";
}

QString TextReader::readFileContent(const QString& filePath, const QString& encoding)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return QString();
    }

    QByteArray data = file.readAll();
    file.close();

    QTextCodec* codec = QTextCodec::codecForName(encoding.toUtf8());
    if (!codec) {
        codec = QTextCodec::codecForName("UTF-8");
    }

    if (encoding == "UTF-8" && data.startsWith("\xEF\xBB\xBF")) {
        data = data.mid(3);
    }

    return codec->toUnicode(data);
}
