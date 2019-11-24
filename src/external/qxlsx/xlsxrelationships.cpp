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
#include "xlsxrelationships_p.h"
#include <QXmlStreamWriter>
#include <QXmlStreamReader>
#include <QDir>
#include <QFile>
#include <QBuffer>

namespace QXlsx {

const QString schema_doc = QStringLiteral("http://schemas.openxmlformats.org/officeDocument/2006/relationships");
const QString schema_msPackage = QStringLiteral("http://schemas.microsoft.com/office/2006/relationships");
const QString schema_package = QStringLiteral("http://schemas.openxmlformats.org/package/2006/relationships");
//const QString schema_worksheet = QStringLiteral("http://schemas.openxmlformats.org/officeDocument/2006/relationships");
Relationships::Relationships()
{
}

QList<XlsxRelationship> Relationships::documentRelationships(const QString &relativeType) const
{
    return relationships(schema_doc + relativeType);
}

void Relationships::addDocumentRelationship(const QString &relativeType, const QString &target)
{
    addRelationship(schema_doc + relativeType, target);
}

QList<XlsxRelationship> Relationships::msPackageRelationships(const QString &relativeType) const
{
    return relationships(schema_msPackage + relativeType);
}

void Relationships::addMsPackageRelationship(const QString &relativeType, const QString &target)
{
    addRelationship(schema_msPackage + relativeType, target);
}

QList<XlsxRelationship> Relationships::packageRelationships(const QString &relativeType) const
{
    return relationships(schema_package + relativeType);
}

void Relationships::addPackageRelationship(const QString &relativeType, const QString &target)
{
    addRelationship(schema_package + relativeType, target);
}

QList<XlsxRelationship> Relationships::worksheetRelationships(const QString &relativeType) const
{
    return relationships(schema_doc + relativeType);
}

void Relationships::addWorksheetRelationship(const QString &relativeType, const QString &target, const QString &targetMode)
{
    addRelationship(schema_doc + relativeType, target, targetMode);
}

QList<XlsxRelationship> Relationships::relationships(const QString &type) const
{
    QList<XlsxRelationship> res;
    foreach (XlsxRelationship ship, m_relationships) {
        if (ship.type == type)
            res.append(ship);
    }
    return res;
}

void Relationships::addRelationship(const QString &type, const QString &target, const QString &targetMode)
{
    XlsxRelationship relation;
    relation.id = QStringLiteral("rId%1").arg(m_relationships.size()+1);
    relation.type = type;
    relation.target = target;
    relation.targetMode = targetMode;

    m_relationships.append(relation);
}

void Relationships::saveToXmlFile(QIODevice *device) const
{
    QXmlStreamWriter writer(device);

    writer.writeStartDocument(QStringLiteral("1.0"), true);
    writer.writeStartElement(QStringLiteral("Relationships"));
    writer.writeAttribute(QStringLiteral("xmlns"), QStringLiteral("http://schemas.openxmlformats.org/package/2006/relationships"));
    foreach (XlsxRelationship relation, m_relationships) {
        writer.writeStartElement(QStringLiteral("Relationship"));
        writer.writeAttribute(QStringLiteral("Id"), relation.id);
        writer.writeAttribute(QStringLiteral("Type"), relation.type);
        writer.writeAttribute(QStringLiteral("Target"), relation.target);
        if (!relation.targetMode.isNull())
            writer.writeAttribute(QStringLiteral("TargetMode"), relation.targetMode);
        writer.writeEndElement();
    }
    writer.writeEndElement();//Relationships
    writer.writeEndDocument();
}

QByteArray Relationships::saveToXmlData() const
{
    QByteArray data;
    QBuffer buffer(&data);
    buffer.open(QIODevice::WriteOnly);
    saveToXmlFile(&buffer);

    return data;
}

bool Relationships::loadFromXmlFile(QIODevice *device)
{
    clear();
    QXmlStreamReader reader(device);
    while (!reader.atEnd()) {
         QXmlStreamReader::TokenType token = reader.readNext();
         if (token == QXmlStreamReader::StartElement) {
             if (reader.name() == QStringLiteral("Relationship")) {
                 QXmlStreamAttributes attributes = reader.attributes();
                 XlsxRelationship relationship;
                 relationship.id = attributes.value(QLatin1String("Id")).toString();
                 relationship.type = attributes.value(QLatin1String("Type")).toString();
                 relationship.target = attributes.value(QLatin1String("Target")).toString();
                 relationship.targetMode = attributes.value(QLatin1String("TargetMode")).toString();
                 m_relationships.append(relationship);
             }
         }

         if (reader.hasError())
            return false;
    }
    return true;
}

bool Relationships::loadFromXmlData(const QByteArray &data)
{
    QBuffer buffer;
    buffer.setData(data);
    buffer.open(QIODevice::ReadOnly);
    return loadFromXmlFile(&buffer);
}

XlsxRelationship Relationships::getRelationshipById(const QString &id) const
{
    foreach (XlsxRelationship ship, m_relationships) {
        if (ship.id == id)
            return ship;
    }
    return XlsxRelationship();
}

void Relationships::clear()
{
    m_relationships.clear();
}

int Relationships::count() const
{
    return m_relationships.count();
}

bool Relationships::isEmpty() const
{
    return m_relationships.isEmpty();
}

} //namespace
