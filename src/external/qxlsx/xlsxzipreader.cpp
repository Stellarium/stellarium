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

#include "xlsxzipreader_p.h"

namespace QXlsx {

ZipReader::ZipReader(const QString &filePath) :
    m_reader(new Stel::QZipReader(filePath))
{
    init();
}

ZipReader::ZipReader(QIODevice *device) :
    m_reader(new Stel::QZipReader(device))
{
    init();
}

ZipReader::~ZipReader()
{
}

void ZipReader::init()
{
	QList<Stel::QZipReader::FileInfo> allFiles = m_reader->fileInfoList();
	foreach (const Stel::QZipReader::FileInfo &fi, allFiles)
	{
		if (fi.isFile)
			m_filePaths.append(fi.filePath);
	}
}

bool ZipReader::exists() const
{
    return m_reader->exists();
}

QStringList ZipReader::filePaths() const
{
    return m_filePaths;
}

QByteArray ZipReader::fileData(const QString &fileName) const
{
    return m_reader->fileData(fileName);
}

} // namespace QXlsx
