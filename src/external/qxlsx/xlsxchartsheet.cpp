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
#include "xlsxchartsheet.h"
#include "xlsxchartsheet_p.h"
#include "xlsxworkbook.h"
#include "xlsxutility_p.h"
#include "xlsxdrawing_p.h"
#include "xlsxdrawinganchor_p.h"
#include "xlsxchart.h"

#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QDir>

QT_BEGIN_NAMESPACE_XLSX

ChartsheetPrivate::ChartsheetPrivate(Chartsheet *p, Chartsheet::CreateFlag flag)
    : AbstractSheetPrivate(p, flag), chart(0)
{

}

ChartsheetPrivate::~ChartsheetPrivate()
{
}

/*!
  \class Chartsheet
  \inmodule QtXlsx
  \brief Represent one chartsheet in the workbook.
*/

/*!
 * \internal
 */
Chartsheet::Chartsheet(const QString &name, int id, Workbook *workbook, CreateFlag flag)
    :AbstractSheet(name, id, workbook, new ChartsheetPrivate(this, flag))
{
    setSheetType(ST_ChartSheet);

    if (flag == Chartsheet::F_NewFromScratch) {
        d_func()->drawing = QSharedPointer<Drawing>(new Drawing(this, flag));

        DrawingAbsoluteAnchor *anchor = new DrawingAbsoluteAnchor(drawing(), DrawingAnchor::Picture);

        anchor->pos = QPoint(0, 0);
        anchor->ext = QSize(9293679, 6068786);

        QSharedPointer<Chart> chart = QSharedPointer<Chart>(new Chart(this, flag));
        chart->setChartType(Chart::CT_Bar);
        anchor->setObjectGraphicFrame(chart);

        d_func()->chart = chart.data();
    }
}

/*!
 * \internal
 *
 * Make a copy of this sheet.
 */

Chartsheet *Chartsheet::copy(const QString &distName, int distId) const
{
    //:Todo
    Q_UNUSED(distName)
    Q_UNUSED(distId)
    return 0;
}

/*!
 * Destroys this workssheet.
 */
Chartsheet::~Chartsheet()
{
}

/*!
 * Returns the chart object of the sheet.
 */
Chart *Chartsheet::chart()
{
    Q_D(Chartsheet);

    return d->chart;
}

void Chartsheet::saveToXmlFile(QIODevice *device) const
{
    Q_D(const Chartsheet);
    d->relationships->clear();

    QXmlStreamWriter writer(device);

    writer.writeStartDocument(QStringLiteral("1.0"), true);
    writer.writeDefaultNamespace(QStringLiteral("http://schemas.openxmlformats.org/spreadsheetml/2006/main"));
    writer.writeNamespace(QStringLiteral("http://schemas.openxmlformats.org/officeDocument/2006/relationships"), QStringLiteral("r"));
    writer.writeStartElement(QStringLiteral("chartsheet"));

    writer.writeStartElement(QStringLiteral("sheetViews"));
    writer.writeEmptyElement(QStringLiteral("sheetView"));
    writer.writeAttribute(QStringLiteral("workbookViewId"), QString::number(0));
    writer.writeAttribute(QStringLiteral("zoomToFit"), QStringLiteral("1"));
    writer.writeEndElement(); //sheetViews

    int idx = d->workbook->drawings().indexOf(d->drawing.data());
    d->relationships->addWorksheetRelationship(QStringLiteral("/drawing"), QStringLiteral("../drawings/drawing%1.xml").arg(idx+1));

    writer.writeEmptyElement(QStringLiteral("drawing"));
    writer.writeAttribute(QStringLiteral("r:id"), QStringLiteral("rId%1").arg(d->relationships->count()));

    writer.writeEndElement();//chartsheet
    writer.writeEndDocument();
}

bool Chartsheet::loadFromXmlFile(QIODevice *device)
{
    Q_D(Chartsheet);

    QXmlStreamReader reader(device);
    while (!reader.atEnd()) {
        reader.readNextStartElement();
        if (reader.tokenType() == QXmlStreamReader::StartElement) {
            if (reader.name() == QLatin1String("drawing")) {
                QString rId = reader.attributes().value(QStringLiteral("r:id")).toString();
                QString name = d->relationships->getRelationshipById(rId).target;
                QString path = QDir::cleanPath(splitPath(filePath())[0] + QLatin1String("/") + name);
                d->drawing = QSharedPointer<Drawing>(new Drawing(this, F_LoadFromExists));
                d->drawing->setFilePath(path);
            }
        }
    }

    return true;
}

QT_END_NAMESPACE_XLSX
