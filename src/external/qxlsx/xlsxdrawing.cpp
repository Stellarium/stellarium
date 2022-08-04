// xlsxdrawing.cpp

#include <QtGlobal>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>
#include <QBuffer>

#include "xlsxdrawing_p.h"
#include "xlsxdrawinganchor_p.h"
#include "xlsxabstractsheet.h"

QT_BEGIN_NAMESPACE_XLSX

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

    for (DrawingAnchor *anchor : anchors)
        anchor->saveToXml(writer);

    writer.writeEndElement();//xdr:wsDr
    writer.writeEndDocument();
}

// check point
bool Drawing::loadFromXmlFile(QIODevice *device)
{
    /*
    <xsd:group name="EG_Anchor">
        <xsd:choice>
            <xsd:element name="twoCellAnchor" type="CT_TwoCellAnchor"/>
            <xsd:element name="oneCellAnchor" type="CT_OneCellAnchor"/>
            <xsd:element name="absoluteAnchor" type="CT_AbsoluteAnchor"/>
        </xsd:choice>
    </xsd:group>
    */

    QXmlStreamReader reader(device);

    while (!reader.atEnd())
    {
        reader.readNextStartElement();
        if (reader.tokenType() == QXmlStreamReader::StartElement)
        {
            if (reader.name() == QLatin1String("absoluteAnchor")) // CT_AbsoluteAnchor
            {
                DrawingAbsoluteAnchor * anchor = new DrawingAbsoluteAnchor(this);
                anchor->loadFromXml(reader);
            }
            else if (reader.name() == QLatin1String("oneCellAnchor")) // CT_OneCellAnchor
            {
                DrawingOneCellAnchor * anchor = new DrawingOneCellAnchor(this);
                anchor->loadFromXml(reader);
            }
            else if (reader.name() == QLatin1String("twoCellAnchor")) // CT_TwoCellAnchor
            {
                DrawingTwoCellAnchor * anchor = new DrawingTwoCellAnchor(this);
                anchor->loadFromXml(reader);
            }
        }
    }

    return true;
}

QT_END_NAMESPACE_XLSX
