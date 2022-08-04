// xlsxmediafile.cpp

#include <QtGlobal>
#include <QCryptographicHash>

#include "xlsxmediafile_p.h"

QT_BEGIN_NAMESPACE_XLSX

MediaFile::MediaFile(const QByteArray &bytes, const QString &suffix, const QString &mimeType)
    : m_contents(bytes), m_suffix(suffix), m_mimeType(mimeType)
      , m_index(0), m_indexValid(false)
{
    m_hashKey = QCryptographicHash::hash(m_contents, QCryptographicHash::Md5);
}

MediaFile::MediaFile(const QString &fileName)
    :m_fileName(fileName), m_index(0), m_indexValid(false)
{

}

void MediaFile::set(const QByteArray &bytes, const QString &suffix, const QString &mimeType)
{
    m_contents = bytes;
    m_suffix = suffix;
    m_mimeType = mimeType;
    m_hashKey = QCryptographicHash::hash(m_contents, QCryptographicHash::Md5);
    m_indexValid = false;
}

void MediaFile::setFileName(const QString &name)
{
    m_fileName = name;
}

QString MediaFile::fileName() const
{
    return m_fileName;
}

QString MediaFile::suffix() const
{
    return m_suffix;
}

QString MediaFile::mimeType() const
{
    return m_mimeType;
}

QByteArray MediaFile::contents() const
{
    return m_contents;
}

int MediaFile::index() const
{
    return m_index;
}

bool MediaFile::isIndexValid() const
{
    return m_indexValid;
}

void MediaFile::setIndex(int idx)
{
    m_index = idx;
    m_indexValid = true;
}

QByteArray MediaFile::hashKey() const
{
    return m_hashKey;
}

QT_END_NAMESPACE_XLSX
