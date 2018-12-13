/****************************************************************************
** Copyright (c) 2013-2014 Debao Zhang <hello@debao.me>
** All right reserved.
**
** Permission is hereby granted, free of charge, to any person obtaining
** a copy of this software and associated documentation files (the
** "Software"), to deal in the Software without restriction, including
** without limitation the rights to use, copy, modify, merge, publish,
** distribute, sublicense, and/or sell copies of the Software, and to
** permit persons to whom the Software is furnished to do so, subject to
** the following conditions:
**
** The above copyright notice and this permission notice shall be
** included in all copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
** NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
** LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
** OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
** WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**
****************************************************************************/

#include "xlsxmediafile_p.h"
#include <QCryptographicHash>

namespace QXlsx {

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

} // namespace QXlsx
