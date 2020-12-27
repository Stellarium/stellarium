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

#ifndef QXLSX_XLSXCOLOR_P_H
#define QXLSX_XLSXCOLOR_P_H

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
#include <QVariant>
#include <QColor>

class QXmlStreamWriter;
class QXmlStreamReader;

namespace QXlsx {

class Styles;

class   XlsxColor
{
public:
    explicit XlsxColor(const QColor &color = QColor());
    explicit XlsxColor(const QString &theme, const QString &tint=QString());
    explicit XlsxColor (int index);

    bool isThemeColor() const;
    bool isIndexedColor() const;
    bool isRgbColor() const;
    bool isInvalid() const;

    QColor rgbColor() const;
    int indexedColor() const;
    QStringList themeColor() const;

    operator QVariant() const;

    static QColor fromARGBString(const QString &c);
    static QString toARGBString(const QColor &c);

    bool saveToXml(QXmlStreamWriter &writer, const QString &node=QString()) const;
    bool loadFromXml(QXmlStreamReader &reader);

private:
    QVariant val;
};

#if !defined(QT_NO_DATASTREAM)
  QDataStream &operator<<(QDataStream &, const XlsxColor &);
  QDataStream &operator>>(QDataStream &, XlsxColor &);
#endif

#ifndef QT_NO_DEBUG_STREAM
  QDebug operator<<(QDebug dbg, const XlsxColor &c);
#endif

} // namespace QXlsx

Q_DECLARE_METATYPE(QXlsx::XlsxColor)

#endif // QXLSX_XLSXCOLOR_P_H
