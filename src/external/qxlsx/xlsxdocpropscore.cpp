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
#include "xlsxdocpropscore_p.h"

#include <QXmlStreamWriter>
#include <QXmlStreamReader>
#include <QDir>
#include <QFile>
#include <QDateTime>
#include <QDebug>
#include <QBuffer>

namespace QXlsx {

DocPropsCore::DocPropsCore(CreateFlag flag)
    :AbstractOOXmlFile(flag)
{
}

bool DocPropsCore::setProperty(const QString &name, const QString &value)
{
    static QStringList validKeys;
    if (validKeys.isEmpty()) {
        validKeys << QStringLiteral("title") << QStringLiteral("subject")
                  << QStringLiteral("keywords") << QStringLiteral("description")
                  << QStringLiteral("category") << QStringLiteral("status")
                  << QStringLiteral("created") << QStringLiteral("creator");
    }

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
    if (m_properties.contains(name))
        return m_properties[name];

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

    if (m_properties.contains(QStringLiteral("title")))
        writer.writeTextElement(dc, QStringLiteral("title"), m_properties[QStringLiteral("title")]);

    if (m_properties.contains(QStringLiteral("subject")))
        writer.writeTextElement(dc, QStringLiteral("subject"), m_properties[QStringLiteral("subject")]);

    writer.writeTextElement(dc, QStringLiteral("creator"), m_properties.contains(QStringLiteral("creator")) ? m_properties[QStringLiteral("creator")] : QStringLiteral("Qt Xlsx Library"));

    if (m_properties.contains(QStringLiteral("keywords")))
        writer.writeTextElement(cp, QStringLiteral("keywords"), m_properties[QStringLiteral("keywords")]);

    if (m_properties.contains(QStringLiteral("description")))
        writer.writeTextElement(dc, QStringLiteral("description"), m_properties[QStringLiteral("description")]);

    writer.writeTextElement(cp, QStringLiteral("lastModifiedBy"), m_properties.contains(QStringLiteral("creator")) ? m_properties[QStringLiteral("creator")] : QStringLiteral("Qt Xlsx Library"));

    writer.writeStartElement(dcterms, QStringLiteral("created"));
    writer.writeAttribute(xsi, QStringLiteral("type"), QStringLiteral("dcterms:W3CDTF"));
    writer.writeCharacters(m_properties.contains(QStringLiteral("created")) ? m_properties[QStringLiteral("created")] : QDateTime::currentDateTime().toString(Qt::ISODate));
    writer.writeEndElement();//dcterms:created

    writer.writeStartElement(dcterms, QStringLiteral("modified"));
    writer.writeAttribute(xsi, QStringLiteral("type"), QStringLiteral("dcterms:W3CDTF"));
    writer.writeCharacters(QDateTime::currentDateTime().toString(Qt::ISODate));
    writer.writeEndElement();//dcterms:created

    if (m_properties.contains(QStringLiteral("category")))
        writer.writeTextElement(cp, QStringLiteral("category"), m_properties[QStringLiteral("category")]);

    if (m_properties.contains(QStringLiteral("status")))
        writer.writeTextElement(cp, QStringLiteral("contentStatus"), m_properties[QStringLiteral("status")]);

    writer.writeEndElement(); //cp:coreProperties
    writer.writeEndDocument();
}

bool DocPropsCore::loadFromXmlFile(QIODevice *device)
{
    QXmlStreamReader reader(device);

    const QString cp = QStringLiteral("http://schemas.openxmlformats.org/package/2006/metadata/core-properties");
    const QString dc = QStringLiteral("http://purl.org/dc/elements/1.1/");
    const QString dcterms = QStringLiteral("http://purl.org/dc/terms/");

    while (!reader.atEnd()) {
         QXmlStreamReader::TokenType token = reader.readNext();
         if (token == QXmlStreamReader::StartElement) {
             const QStringRef nsUri = reader.namespaceUri();
             const QStringRef name = reader.name();
             if (name == QStringLiteral("subject") && nsUri == dc) {
                 setProperty(QStringLiteral("subject"), reader.readElementText());
             } else if (name == QStringLiteral("title") && nsUri == dc) {
                 setProperty(QStringLiteral("title"), reader.readElementText());
             } else if (name == QStringLiteral("creator") && nsUri == dc) {
                 setProperty(QStringLiteral("creator"), reader.readElementText());
             } else if (name == QStringLiteral("description") && nsUri == dc) {
                 setProperty(QStringLiteral("description"), reader.readElementText());
             } else if (name == QStringLiteral("keywords") && nsUri == cp) {
                 setProperty(QStringLiteral("keywords"), reader.readElementText());
             } else if (name == QStringLiteral("created") && nsUri == dcterms) {
                 setProperty(QStringLiteral("created"), reader.readElementText());
             } else if (name == QStringLiteral("category") && nsUri == cp) {
                 setProperty(QStringLiteral("category"), reader.readElementText());
             } else if (name == QStringLiteral("contentStatus") && nsUri == cp) {
                 setProperty(QStringLiteral("status"), reader.readElementText());
             }
         }

         if (reader.hasError()) {
             qDebug()<<"Error when read doc props core file."<<reader.errorString();

         }
    }
    return true;
}

} //namespace
