// xlsxcontenttypes.cpp

#include <QXmlStreamWriter>
#include <QXmlStreamReader>
#include <QFile>
#include <QMapIterator>
#include <QBuffer>
#include <QDebug>

#include "xlsxcontenttypes_p.h"

QT_BEGIN_NAMESPACE_XLSX

ContentTypes::ContentTypes(CreateFlag flag)
    :AbstractOOXmlFile(flag)
{
    m_package_prefix = QStringLiteral("application/vnd.openxmlformats-package.");
    m_document_prefix = QStringLiteral("application/vnd.openxmlformats-officedocument.");

    m_defaults.insert(QStringLiteral("rels"), m_package_prefix + QLatin1String("relationships+xml"));
    m_defaults.insert(QStringLiteral("xml"), QStringLiteral("application/xml"));
}

void ContentTypes::addDefault(const QString &key, const QString &value)
{
    m_defaults.insert(key, value);
}

void ContentTypes::addOverride(const QString &key, const QString &value)
{
    m_overrides.insert(key, value);
}

void ContentTypes::addDocPropApp()
{
    addOverride(QStringLiteral("/docProps/app.xml"), m_document_prefix + QLatin1String("extended-properties+xml"));
}

void ContentTypes::addDocPropCore()
{
    addOverride(QStringLiteral("/docProps/core.xml"), m_package_prefix + QLatin1String("core-properties+xml"));
}

void ContentTypes::addStyles()
{
    addOverride(QStringLiteral("/xl/styles.xml"), m_document_prefix + QLatin1String("spreadsheetml.styles+xml"));
}

void ContentTypes::addTheme()
{
    addOverride(QStringLiteral("/xl/theme/theme1.xml"), m_document_prefix + QLatin1String("theme+xml"));
}

void ContentTypes::addWorkbook()
{
    addOverride(QStringLiteral("/xl/workbook.xml"), m_document_prefix + QLatin1String("spreadsheetml.sheet.main+xml"));
}

void ContentTypes::addWorksheetName(const QString &name)
{
    addOverride(QStringLiteral("/xl/worksheets/%1.xml").arg(name), m_document_prefix + QLatin1String("spreadsheetml.worksheet+xml"));
}

void ContentTypes::addChartsheetName(const QString &name)
{
    addOverride(QStringLiteral("/xl/chartsheets/%1.xml").arg(name), m_document_prefix + QLatin1String("spreadsheetml.chartsheet+xml"));
}

void ContentTypes::addDrawingName(const QString &name)
{
    addOverride(QStringLiteral("/xl/drawings/%1.xml").arg(name), m_document_prefix + QLatin1String("drawing+xml"));
}

void ContentTypes::addChartName(const QString &name)
{
    addOverride(QStringLiteral("/xl/charts/%1.xml").arg(name), m_document_prefix + QLatin1String("drawingml.chart+xml"));
}

void ContentTypes::addCommentName(const QString &name)
{
    addOverride(QStringLiteral("/xl/%1.xml").arg(name), m_document_prefix + QLatin1String("spreadsheetml.comments+xml"));
}

void ContentTypes::addTableName(const QString &name)
{
    addOverride(QStringLiteral("/xl/tables/%1.xml").arg(name), m_document_prefix + QLatin1String("spreadsheetml.table+xml"));
}

void ContentTypes::addExternalLinkName(const QString &name)
{
    addOverride(QStringLiteral("/xl/externalLinks/%1.xml").arg(name), m_document_prefix + QLatin1String("spreadsheetml.externalLink+xml"));
}

void ContentTypes::addSharedString()
{
    addOverride(QStringLiteral("/xl/sharedStrings.xml"), m_document_prefix + QLatin1String("spreadsheetml.sharedStrings+xml"));
}

void ContentTypes::addVmlName()
{
    addOverride(QStringLiteral("vml"), m_document_prefix + QLatin1String("vmlDrawing"));
}

void ContentTypes::addCalcChain()
{
    addOverride(QStringLiteral("/xl/calcChain.xml"), m_document_prefix + QLatin1String("spreadsheetml.calcChain+xml"));
}

void ContentTypes::addVbaProject()
{
    //:TODO
    addOverride(QStringLiteral("bin"), QStringLiteral("application/vnd.ms-office.vbaProject"));
}

void ContentTypes::clearOverrides()
{
    m_overrides.clear();
}

void ContentTypes::saveToXmlFile(QIODevice *device) const
{
    QXmlStreamWriter writer(device);

    writer.writeStartDocument(QStringLiteral("1.0"), true);
    writer.writeStartElement(QStringLiteral("Types"));
    writer.writeAttribute(QStringLiteral("xmlns"), QStringLiteral("http://schemas.openxmlformats.org/package/2006/content-types"));

    {
    QMapIterator<QString, QString> it(m_defaults);
    while (it.hasNext()) {
        it.next();
        writer.writeStartElement(QStringLiteral("Default"));
        writer.writeAttribute(QStringLiteral("Extension"), it.key());
        writer.writeAttribute(QStringLiteral("ContentType"), it.value());
        writer.writeEndElement();//Default
    }
    }

    {
    QMapIterator<QString, QString> it(m_overrides);
    while (it.hasNext()) {
        it.next();
        writer.writeStartElement(QStringLiteral("Override"));
        writer.writeAttribute(QStringLiteral("PartName"), it.key());
        writer.writeAttribute(QStringLiteral("ContentType"), it.value());
        writer.writeEndElement(); //Override
    }
    }

    writer.writeEndElement();//Types
    writer.writeEndDocument();

}

bool ContentTypes::loadFromXmlFile(QIODevice *device)
{
    m_defaults.clear();
    m_overrides.clear();

    QXmlStreamReader reader(device);
    while (!reader.atEnd()) {
        QXmlStreamReader::TokenType token = reader.readNext();
        if (token == QXmlStreamReader::StartElement) {
            if (reader.name() == QLatin1String("Default")) {
                QXmlStreamAttributes attrs = reader.attributes();
                QString extension = attrs.value(QLatin1String("Extension")).toString();
                QString type = attrs.value(QLatin1String("ContentType")).toString();
                m_defaults.insert(extension, type);
            } else if (reader.name() == QLatin1String("Override")) {
                QXmlStreamAttributes attrs = reader.attributes();
                QString partName = attrs.value(QLatin1String("PartName")).toString();
                QString type = attrs.value(QLatin1String("ContentType")).toString();
                m_overrides.insert(partName, type);
            }
        }

        if (reader.hasError()) {
            qDebug()<<reader.errorString();
        }
    }
    return true;
}

QT_END_NAMESPACE_XLSX
