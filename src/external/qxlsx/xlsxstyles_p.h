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
#ifndef XLSXSTYLES_H
#define XLSXSTYLES_H

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
#include "xlsxformat.h"
#include "xlsxabstractooxmlfile.h"
#include <QSharedPointer>
#include <QHash>
#include <QList>
#include <QMap>
#include <QStringList>
#include <QVector>

class QXmlStreamWriter;
class QXmlStreamReader;
class QIODevice;
class StylesTest;

namespace QXlsx {

class Format;
class XlsxColor;

struct XlsxFormatNumberData
{
    XlsxFormatNumberData() : formatIndex(0) {}

    int formatIndex;
    QString formatString;
};

class  Styles : public AbstractOOXmlFile
{
public:
    Styles(CreateFlag flag);
    ~Styles();
    void addXfFormat(const Format &format, bool force=false);
    Format xfFormat(int idx) const;
    void addDxfFormat(const Format &format, bool force=false);
    Format dxfFormat(int idx) const;

    void saveToXmlFile(QIODevice *device) const;
    bool loadFromXmlFile(QIODevice *device);

    QColor getColorByIndex(int idx);

private:
    friend class Format;
    friend class ::StylesTest;

    void fixNumFmt(const Format &format);

    void writeNumFmts(QXmlStreamWriter &writer) const;
    void writeFonts(QXmlStreamWriter &writer) const;
    void writeFont(QXmlStreamWriter &writer, const Format &font, bool isDxf = false) const;
    void writeFills(QXmlStreamWriter &writer) const;
    void writeFill(QXmlStreamWriter &writer, const Format &fill, bool isDxf = false) const;
    void writeBorders(QXmlStreamWriter &writer) const;
    void writeBorder(QXmlStreamWriter &writer, const Format &border, bool isDxf = false) const;
    void writeSubBorder(QXmlStreamWriter &writer, const QString &type, int style, const XlsxColor &color) const;
    void writeCellXfs(QXmlStreamWriter &writer) const;
    void writeDxfs(QXmlStreamWriter &writer) const;
    void writeDxf(QXmlStreamWriter &writer, const Format &format) const;
    void writeColors(QXmlStreamWriter &writer) const;

    bool readNumFmts(QXmlStreamReader &reader);
    bool readFonts(QXmlStreamReader &reader);
    bool readFont(QXmlStreamReader &reader, Format &format);
    bool readFills(QXmlStreamReader &reader);
    bool readFill(QXmlStreamReader &reader, Format &format);
    bool readBorders(QXmlStreamReader &reader);
    bool readBorder(QXmlStreamReader &reader, Format &format);
    bool readSubBorder(QXmlStreamReader &reader, const QString &name, Format::BorderStyle &style, XlsxColor &color);
    bool readCellXfs(QXmlStreamReader &reader);
    bool readDxfs(QXmlStreamReader &reader);
    bool readDxf(QXmlStreamReader &reader);
    bool readColors(QXmlStreamReader &reader);
    bool readIndexedColors(QXmlStreamReader &reader);

    QHash<QString, int> m_builtinNumFmtsHash;
    QMap<int, QSharedPointer<XlsxFormatNumberData> > m_customNumFmtIdMap;
    QHash<QString, QSharedPointer<XlsxFormatNumberData> > m_customNumFmtsHash;
    int m_nextCustomNumFmtId;
    QList<Format> m_fontsList;
    QList<Format> m_fillsList;
    QList<Format> m_bordersList;
    QHash<QByteArray, Format> m_fontsHash;
    QHash<QByteArray, Format> m_fillsHash;
    QHash<QByteArray, Format> m_bordersHash;

    QVector<QColor> m_indexedColors;
    bool m_isIndexedColorsDefault;

    QList<Format> m_xf_formatsList;
    QHash<QByteArray, Format> m_xf_formatsHash;

    QList<Format> m_dxf_formatsList;
    QHash<QByteArray, Format> m_dxf_formatsHash;

    bool m_emptyFormatAdded;
};

}
#endif // XLSXSTYLES_H
