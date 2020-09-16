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
#ifndef XLSXSHAREDSTRINGS_H
#define XLSXSHAREDSTRINGS_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt Xlsx API.  It exists for the convenience
// of the Qt Xlsx.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include "xlsxglobal.h"
#include "xlsxrichstring.h"
#include "xlsxabstractooxmlfile.h"
#include <QHash>
#include <QStringList>
#include <QSharedPointer>

class QIODevice;
class QXmlStreamReader;
class QXmlStreamWriter;

namespace QXlsx {

class XlsxSharedStringInfo
{
public:
    XlsxSharedStringInfo(int index=0, int count = 1) :
        index(index), count(count)
    {
    }

    int index;
    int count;
};

class  SharedStrings : public AbstractOOXmlFile
{
public:
    SharedStrings(CreateFlag flag);
    int count() const;
    bool isEmpty() const;
    
    int addSharedString(const QString &string);
    int addSharedString(const RichString &string);
    void removeSharedString(const QString &string);
    void removeSharedString(const RichString &string);
    void incRefByStringIndex(int idx);

    int getSharedStringIndex(const QString &string) const;
    int getSharedStringIndex(const RichString &string) const;
    RichString getSharedString(int index) const;
    QList<RichString> getSharedStrings() const;

    void saveToXmlFile(QIODevice *device) const;
    bool loadFromXmlFile(QIODevice *device);

private:
    void readString(QXmlStreamReader &reader); // <si>
    void readRichStringPart(QXmlStreamReader &reader, RichString &rich); // <r>
    void readPlainStringPart(QXmlStreamReader &reader, RichString &rich); // <v>
    Format readRichStringPart_rPr(QXmlStreamReader &reader);
    void writeRichStringPart_rPr(QXmlStreamWriter &writer, const Format &format) const;

    QHash<RichString, XlsxSharedStringInfo> m_stringTable; //for fast lookup
    QList<RichString> m_stringList;
    int m_stringCount;
};

}
#endif // XLSXSHAREDSTRINGS_H
