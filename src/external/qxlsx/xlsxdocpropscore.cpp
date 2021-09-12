// xlsxdocpropscore.cpp

#include <QtGlobal>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>
#include <QDir>
#include <QFile>
#include <QDateTime>
#include <QDebug>
#include <QBuffer>

#include "xlsxdocpropscore_p.h"

QT_BEGIN_NAMESPACE_XLSX

DocPropsCore::DocPropsCore(CreateFlag flag)
    :AbstractOOXmlFile(flag)
{
}

bool DocPropsCore::setProperty(const QString &name, const QString &value)
{
    static const QStringList validKeys = {
        QStringLiteral("title"), QStringLiteral("subject"),
        QStringLiteral("keywords"), QStringLiteral("description"),
        QStringLiteral("category"), QStringLiteral("status"),
        QStringLiteral("created"), QStringLiteral("creator")
    };

    if (!validKeys.contains(name))
        return false;

    if (value.isEmpty())
        m_properties.remove(name);
    else
        m_properties[name] = value;

    return true;
}

QString DocPropsCore::property(const QString &name) const
{
    auto it = m_properties.constFind(name);
    if (it != m_properties.constEnd())
        return it.value();

    return QString();
}

QStringList DocPropsCore::propertyNames() const
{
    return m_properties.keys();
}

void DocPropsCore::saveToXmlFile(QIODevice *device) const
{
    QXmlStreamWriter writer(device);
    const QString cp = QStringLiteral("http://schemas.openxmlformats.org/package/2006/metadata/core-properties");
    const QString dc = QStringLiteral("http://purl.org/dc/elements/1.1/");
    const QString dcterms = QStringLiteral("http://purl.org/dc/terms/");
    const QString dcmitype = QStringLiteral("http://purl.org/dc/dcmitype/");
    const QString xsi = QStringLiteral("http://www.w3.org/2001/XMLSchema-instance");
    writer.writeStartDocument(QStringLiteral("1.0"), true);
    writer.writeStartElement(QStringLiteral("cp:coreProperties"));
    writer.writeNamespace(cp, QStringLiteral("cp"));
    writer.writeNamespace(dc, QStringLiteral("dc"));
    writer.writeNamespace(dcterms, QStringLiteral("dcterms"));
    writer.writeNamespace(dcmitype, QStringLiteral("dcmitype"));
    writer.writeNamespace(xsi, QStringLiteral("xsi"));

    auto it = m_properties.constFind(QStringLiteral("title"));
    if (it != m_properties.constEnd())
        writer.writeTextElement(dc, QStringLiteral("title"), it.value());

    it = m_properties.constFind(QStringLiteral("subject"));
    if (it != m_properties.constEnd())
        writer.writeTextElement(dc, QStringLiteral("subject"), it.value());

    it = m_properties.constFind(QStringLiteral("creator"));
    writer.writeTextElement(dc, QStringLiteral("creator"), it != m_properties.constEnd() ? it.value() : QStringLiteral("Qt Xlsx Library"));

    it = m_properties.constFind(QStringLiteral("keywords"));
    if (it != m_properties.constEnd())
        writer.writeTextElement(cp, QStringLiteral("keywords"), it.value());

    it = m_properties.constFind(QStringLiteral("description"));
    if (it != m_properties.constEnd())
        writer.writeTextElement(dc, QStringLiteral("description"), it.value());

    it = m_properties.constFind(QStringLiteral("creator"));
    writer.writeTextElement(cp, QStringLiteral("lastModifiedBy"), it != m_properties.constEnd() ? it.value() : QStringLiteral("Qt Xlsx Library"));

    writer.writeStartElement(dcterms, QStringLiteral("created"));
    writer.writeAttribute(xsi, QStringLiteral("type"), QStringLiteral("dcterms:W3CDTF"));
    it = m_properties.constFind(QStringLiteral("created"));
    writer.writeCharacters(it != m_properties.constEnd() ? it.value() : QDateTime::currentDateTime().toString(Qt::ISODate));
    writer.writeEndElement();//dcterms:created

    writer.writeStartElement(dcterms, QStringLiteral("modified"));
    writer.writeAttribute(xsi, QStringLiteral("type"), QStringLiteral("dcterms:W3CDTF"));
    writer.writeCharacters(QDateTime::currentDateTime().toString(Qt::ISODate));
    writer.writeEndElement();//dcterms:created

    it = m_properties.constFind(QStringLiteral("category"));
    if (it != m_properties.constEnd())
        writer.writeTextElement(cp, QStringLiteral("category"), it.value());

    it = m_properties.constFind(QStringLiteral("status"));
    if (it != m_properties.constEnd())
        writer.writeTextElement(cp, QStringLiteral("contentStatus"), it.value());

    writer.writeEndElement(); //cp:coreProperties
    writer.writeEndDocument();
}

bool DocPropsCore::loadFromXmlFile(QIODevice *device)
{
    QXmlStreamReader reader(device);

    const QString cp = QStringLiteral("http://schemas.openxmlformats.org/package/2006/metadata/core-properties");
    const QString dc = QStringLiteral("http://purl.org/dc/elements/1.1/");
    const QString dcterms = QStringLiteral("http://purl.org/dc/terms/");

    while (!reader.atEnd())
    {
         QXmlStreamReader::TokenType token = reader.readNext();

         if (token == QXmlStreamReader::StartElement)
         {

             const auto& nsUri = reader.namespaceUri();
             const auto& name = reader.name();

             if (name == QStringLiteral("subject") && nsUri == dc)
             {
                 setProperty(QStringLiteral("subject"), reader.readElementText());
             }
             else if (name == QStringLiteral("title") && nsUri == dc)
             {
                 setProperty(QStringLiteral("title"), reader.readElementText());
             }
             else if (name == QStringLiteral("creator") && nsUri == dc)
             {
                 setProperty(QStringLiteral("creator"), reader.readElementText());
             }
             else if (name == QStringLiteral("description") && nsUri == dc)
             {
                 setProperty(QStringLiteral("description"), reader.readElementText());
             }
             else if (name == QStringLiteral("keywords") && nsUri == cp)
             {
                 setProperty(QStringLiteral("keywords"), reader.readElementText());
             }
             else if (name == QStringLiteral("created") && nsUri == dcterms)
             {
                 setProperty(QStringLiteral("created"), reader.readElementText());
             }
             else if (name == QStringLiteral("category") && nsUri == cp)
             {
                 setProperty(QStringLiteral("category"), reader.readElementText());
             }
             else if (name == QStringLiteral("contentStatus") && nsUri == cp)
             {
                 setProperty(QStringLiteral("status"), reader.readElementText());
             }
         }

         if (reader.hasError())
         {
             qDebug() << "Error when read doc props core file." << reader.errorString();
         }
    }
    return true;
}

QT_END_NAMESPACE_XLSX
