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

#include "xlsxdrawing_p.h"
#include "xlsxdrawinganchor_p.h"
#include "xlsxabstractsheet.h"

#include <QXmlStreamWriter>
#include <QXmlStreamReader>
#include <QBuffer>

namespace QXlsx {

Drawing::Drawing(AbstractSheet *sheet, CreateFlag flag)
    :AbstractOOXmlFile(flag), sheet(sheet)
{
    workbook = sheet->workbook();
}

Drawing::~Drawing()
{
    qDeleteAll(anchors);
}

void Drawing::saveToXmlFile(QIODevice *device) const
{
    relationships()->clear();

    QXmlStreamWriter writer(device);

    writer.writeStartDocument(QStringLiteral("1.0"), true);
    writer.writeStartElement(QStringLiteral("xdr:wsDr"));
    writer.writeAttribute(QStringLiteral("xmlns:xdr"), QStringLiteral("http://schemas.openxmlformats.org/drawingml/2006/spreadsheetDrawing"));
    writer.writeAttribute(QStringLiteral("xmlns:a"), QStringLiteral("http://schemas.openxmlformats.org/drawingml/2006/main"));

    foreach (DrawingAnchor *anchor, anchors)
        anchor->saveToXml(writer);

    writer.writeEndElement();//xdr:wsDr
    writer.writeEndDocument();
}

bool Drawing::loadFromXmlFile(QIODevice *device)
{
    QXmlStreamReader reader(device);
    while (!reader.atEnd()) {
        reader.readNextStartElement();
        if (reader.tokenType() == QXmlStreamReader::StartElement) {
            if (reader.name() == QLatin1String("absoluteAnchor")) {
                DrawingAbsoluteAnchor * anchor = new DrawingAbsoluteAnchor(this);
                anchor->loadFromXml(reader);
            } else if (reader.name() == QLatin1String("oneCellAnchor")) {
                DrawingOneCellAnchor * anchor = new DrawingOneCellAnchor(this);
                anchor->loadFromXml(reader);
            } else if (reader.name() == QLatin1String("twoCellAnchor")) {
                DrawingTwoCellAnchor * anchor = new DrawingTwoCellAnchor(this);
                anchor->loadFromXml(reader);
            }
        }
    }

    return true;
}

} // namespace QXlsx
