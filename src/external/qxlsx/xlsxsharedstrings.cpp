// xlsxsharedstrings.cpp

#include <QtGlobal>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>
#include <QDir>
#include <QFile>
#include <QDebug>
#include <QBuffer>

#include "xlsxrichstring.h"
#include "xlsxsharedstrings_p.h"
#include "xlsxutility_p.h"
#include "xlsxformat_p.h"
#include "xlsxcolor_p.h"

QT_BEGIN_NAMESPACE_XLSX

/*
 * Note that, when we open an existing .xlsx file (broken file?),
 * duplicated string items may exist in the shared string table.
 *
 * In such case, the size of stringList will larger than stringTable.
 * Duplicated items can be removed once we loaded all the worksheets.
 */

SharedStrings::SharedStrings(CreateFlag flag)
    :AbstractOOXmlFile(flag)
{
    m_stringCount = 0;
}

int SharedStrings::count() const
{
    return m_stringCount;
}

bool SharedStrings::isEmpty() const
{
    return m_stringList.isEmpty();
}

int SharedStrings::addSharedString(const QString &string)
{
    return addSharedString(RichString(string));
}

int SharedStrings::addSharedString(const RichString &string)
{
    m_stringCount += 1;

    auto it = m_stringTable.find(string);
    if (it != m_stringTable.end()) {
        it->count += 1;
        return it->index;
    }

    int index = m_stringList.size();
    m_stringTable[string] = XlsxSharedStringInfo(index);
    m_stringList.append(string);
    return index;
}

void SharedStrings::incRefByStringIndex(int idx)
{
    if (idx <0 || idx >= m_stringList.size()) {
        qDebug("SharedStrings: invlid index");
        return;
    }

    addSharedString(m_stringList[idx]);
}

/*
 * Broken, don't use.
 */
void SharedStrings::removeSharedString(const QString &string)
{
    removeSharedString(RichString(string));
}

/*
 * Broken, don't use.
 */
void SharedStrings::removeSharedString(const RichString &string)
{
    auto it = m_stringTable.find(string);
    if (it == m_stringTable.end())
        return;

    m_stringCount -= 1;

    it->count -= 1;

    if (it->count <= 0) {
        for (int i=it->index+1; i<m_stringList.size(); ++i)
            m_stringTable[m_stringList[i]].index -= 1;

        m_stringList.removeAt(it->index);
        m_stringTable.remove(string);
    }
}

int SharedStrings::getSharedStringIndex(const QString &string) const
{
    return getSharedStringIndex(RichString(string));
}

int SharedStrings::getSharedStringIndex(const RichString &string) const
{
    auto it = m_stringTable.constFind(string);
    if (it != m_stringTable.constEnd())
        return it->index;
    return -1;
}

RichString SharedStrings::getSharedString(int index) const
{
    if (index < m_stringList.count() && index >= 0)
        return m_stringList[index];
    return RichString();
}

QList<RichString> SharedStrings::getSharedStrings() const
{
    return m_stringList;
}

void SharedStrings::writeRichStringPart_rPr(QXmlStreamWriter &writer, const Format &format) const
{
    if (!format.hasFontData())
        return;

    if (format.fontBold())
        writer.writeEmptyElement(QStringLiteral("b"));
    if (format.fontItalic())
        writer.writeEmptyElement(QStringLiteral("i"));
    if (format.fontStrikeOut())
        writer.writeEmptyElement(QStringLiteral("strike"));
    if (format.fontOutline())
        writer.writeEmptyElement(QStringLiteral("outline"));
    if (format.boolProperty(FormatPrivate::P_Font_Shadow))
        writer.writeEmptyElement(QStringLiteral("shadow"));
    if (format.hasProperty(FormatPrivate::P_Font_Underline)) {
        Format::FontUnderline u = format.fontUnderline();
        if (u != Format::FontUnderlineNone) {
            writer.writeEmptyElement(QStringLiteral("u"));
            if (u== Format::FontUnderlineDouble)
                writer.writeAttribute(QStringLiteral("val"), QStringLiteral("double"));
            else if (u == Format::FontUnderlineSingleAccounting)
                writer.writeAttribute(QStringLiteral("val"), QStringLiteral("singleAccounting"));
            else if (u == Format::FontUnderlineDoubleAccounting)
                writer.writeAttribute(QStringLiteral("val"), QStringLiteral("doubleAccounting"));
        }
    }
    if (format.hasProperty(FormatPrivate::P_Font_Script)) {
        Format::FontScript s = format.fontScript();
        if (s != Format::FontScriptNormal) {
            writer.writeEmptyElement(QStringLiteral("vertAlign"));
            if (s == Format::FontScriptSuper)
                writer.writeAttribute(QStringLiteral("val"), QStringLiteral("superscript"));
            else
                writer.writeAttribute(QStringLiteral("val"), QStringLiteral("subscript"));
        }
    }

    if (format.hasProperty(FormatPrivate::P_Font_Size)) {
        writer.writeEmptyElement(QStringLiteral("sz"));
        writer.writeAttribute(QStringLiteral("val"), QString::number(format.fontSize()));
    }

    if (format.hasProperty(FormatPrivate::P_Font_Color)) {
        XlsxColor color = format.property(FormatPrivate::P_Font_Color).value<XlsxColor>();
        color.saveToXml(writer);
    }

    if (!format.fontName().isEmpty()) {
        writer.writeEmptyElement(QStringLiteral("rFont"));
        writer.writeAttribute(QStringLiteral("val"), format.fontName());
    }
    if (format.hasProperty(FormatPrivate::P_Font_Family)) {
        writer.writeEmptyElement(QStringLiteral("family"));
        writer.writeAttribute(QStringLiteral("val"), QString::number(format.intProperty(FormatPrivate::P_Font_Family)));
    }

    if (format.hasProperty(FormatPrivate::P_Font_Scheme)) {
        writer.writeEmptyElement(QStringLiteral("scheme"));
        writer.writeAttribute(QStringLiteral("val"), format.stringProperty(FormatPrivate::P_Font_Scheme));
    }
}

void SharedStrings::saveToXmlFile(QIODevice *device) const
{
    QXmlStreamWriter writer(device);

    if (m_stringList.size() != m_stringTable.size()) {
        //Duplicated string items exist in m_stringList
        //Clean up can not be done here, as the indices
        //have been used when we save the worksheets part.
    }

    writer.writeStartDocument(QStringLiteral("1.0"), true);
    writer.writeStartElement(QStringLiteral("sst"));
    writer.writeAttribute(QStringLiteral("xmlns"), QStringLiteral("http://schemas.openxmlformats.org/spreadsheetml/2006/main"));
    writer.writeAttribute(QStringLiteral("count"), QString::number(m_stringCount));
    writer.writeAttribute(QStringLiteral("uniqueCount"), QString::number(m_stringList.size()));

    for (const RichString &string : m_stringList) {
        writer.writeStartElement(QStringLiteral("si"));
        if (string.isRichString()) {
            //Rich text string
            for (int i=0; i<string.fragmentCount(); ++i) {
                writer.writeStartElement(QStringLiteral("r"));
                if (string.fragmentFormat(i).hasFontData()) {
                    writer.writeStartElement(QStringLiteral("rPr"));
                    writeRichStringPart_rPr(writer, string.fragmentFormat(i));
                    writer.writeEndElement();// rPr
                }
                writer.writeStartElement(QStringLiteral("t"));
                if (isSpaceReserveNeeded(string.fragmentText(i)))
                    writer.writeAttribute(QStringLiteral("xml:space"), QStringLiteral("preserve"));
                writer.writeCharacters(string.fragmentText(i));
                writer.writeEndElement();// t

                writer.writeEndElement(); //r
            }
        } else {
            writer.writeStartElement(QStringLiteral("t"));
            QString pString = string.toPlainString();
            if (isSpaceReserveNeeded(pString))
                writer.writeAttribute(QStringLiteral("xml:space"), QStringLiteral("preserve"));
            writer.writeCharacters(pString);
            writer.writeEndElement();//t
        }
        writer.writeEndElement();//si
    }

    writer.writeEndElement(); //sst
    writer.writeEndDocument();
}

void SharedStrings::readString(QXmlStreamReader &reader)
{
    Q_ASSERT(reader.name() == QLatin1String("si"));

    RichString richString;

    while (!reader.atEnd() && !(reader.name() == QLatin1String("si") && reader.tokenType() == QXmlStreamReader::EndElement)) {
        reader.readNextStartElement();
        if (reader.tokenType() == QXmlStreamReader::StartElement) {
            if (reader.name() == QLatin1String("r"))
                readRichStringPart(reader, richString);
            else if (reader.name() == QLatin1String("t"))
                readPlainStringPart(reader, richString);
        }
    }

    int idx = m_stringList.size();
    m_stringTable[richString] = XlsxSharedStringInfo(idx, 0);
    m_stringList.append(richString);
}

void SharedStrings::readRichStringPart(QXmlStreamReader &reader, RichString &richString)
{
    Q_ASSERT(reader.name() == QLatin1String("r"));

    QString text;
    Format format;
    while (!reader.atEnd() && !(reader.name() == QLatin1String("r") && reader.tokenType() == QXmlStreamReader::EndElement)) {
        reader.readNextStartElement();
        if (reader.tokenType() == QXmlStreamReader::StartElement) {
            if (reader.name() == QLatin1String("rPr")) {
                format = readRichStringPart_rPr(reader);
            } else if (reader.name() == QLatin1String("t")) {
                text = reader.readElementText();
            }
        }
    }
    richString.addFragment(text, format);
}

void SharedStrings::readPlainStringPart(QXmlStreamReader &reader, RichString &richString)
{
    Q_ASSERT(reader.name() == QLatin1String("t"));

    //QXmlStreamAttributes attributes = reader.attributes();

	// NOTICE: CHECK POINT
    QString text = reader.readElementText();
    richString.addFragment(text, Format());
}

Format SharedStrings::readRichStringPart_rPr(QXmlStreamReader &reader)
{
    Q_ASSERT(reader.name() == QLatin1String("rPr"));
    Format format;
    while (!reader.atEnd() && !(reader.name() == QLatin1String("rPr") && reader.tokenType() == QXmlStreamReader::EndElement)) {
        reader.readNextStartElement();
        if (reader.tokenType() == QXmlStreamReader::StartElement) {
            QXmlStreamAttributes attributes = reader.attributes();
            if (reader.name() == QLatin1String("rFont")) {
                format.setFontName(attributes.value(QLatin1String("val")).toString());
            } else if (reader.name() == QLatin1String("charset")) {
                format.setProperty(FormatPrivate::P_Font_Charset, attributes.value(QLatin1String("val")).toString().toInt());
            } else if (reader.name() == QLatin1String("family")) {
                format.setProperty(FormatPrivate::P_Font_Family, attributes.value(QLatin1String("val")).toString().toInt());
            } else if (reader.name() == QLatin1String("b")) {
                format.setFontBold(true);
            } else if (reader.name() == QLatin1String("i")) {
                format.setFontItalic(true);
            } else if (reader.name() == QLatin1String("strike")) {
                format.setFontStrikeOut(true);
            } else if (reader.name() == QLatin1String("outline")) {
                format.setFontOutline(true);
            } else if (reader.name() == QLatin1String("shadow")) {
                format.setProperty(FormatPrivate::P_Font_Shadow, true);
            } else if (reader.name() == QLatin1String("condense")) {
                format.setProperty(FormatPrivate::P_Font_Condense, attributes.value(QLatin1String("val")).toString().toInt());
            } else if (reader.name() == QLatin1String("extend")) {
                format.setProperty(FormatPrivate::P_Font_Extend, attributes.value(QLatin1String("val")).toString().toInt());
            } else if (reader.name() == QLatin1String("color")) {
                XlsxColor color;
                color.loadFromXml(reader);
                format.setProperty(FormatPrivate::P_Font_Color, color);
            } else if (reader.name() == QLatin1String("sz")) {
                format.setFontSize(attributes.value(QLatin1String("val")).toString().toInt());
            } else if (reader.name() == QLatin1String("u")) {
                QString value = attributes.value(QLatin1String("val")).toString();
                if (value == QLatin1String("double"))
                    format.setFontUnderline(Format::FontUnderlineDouble);
                else if (value == QLatin1String("doubleAccounting"))
                    format.setFontUnderline(Format::FontUnderlineDoubleAccounting);
                else if (value == QLatin1String("singleAccounting"))
                    format.setFontUnderline(Format::FontUnderlineSingleAccounting);
                else
                    format.setFontUnderline(Format::FontUnderlineSingle);
            } else if (reader.name() == QLatin1String("vertAlign")) {
                QString value = attributes.value(QLatin1String("val")).toString();
                if (value == QLatin1String("superscript"))
                    format.setFontScript(Format::FontScriptSuper);
                else if (value == QLatin1String("subscript"))
                    format.setFontScript(Format::FontScriptSub);
            } else if (reader.name() == QLatin1String("scheme")) {
                format.setProperty(FormatPrivate::P_Font_Scheme, attributes.value(QLatin1String("val")).toString());
            }
        }
    }
    return format;
}

bool SharedStrings::loadFromXmlFile(QIODevice *device)
{
    QXmlStreamReader reader(device);
    int count = 0;
    bool hasUniqueCountAttr=true;
    while (!reader.atEnd()) {
         QXmlStreamReader::TokenType token = reader.readNext();
         if (token == QXmlStreamReader::StartElement) {
             if (reader.name() == QLatin1String("sst")) {
                 QXmlStreamAttributes attributes = reader.attributes();
                 if ((hasUniqueCountAttr = attributes.hasAttribute(QLatin1String("uniqueCount"))))
                     count = attributes.value(QLatin1String("uniqueCount")).toString().toInt();
             } else if (reader.name() == QLatin1String("si")) {
                 readString(reader);
             }
         }
    }

    if (hasUniqueCountAttr && m_stringList.size() != count) {
        qDebug("Error: Shared string count");
        return false;
    }

    if (m_stringList.size() != m_stringTable.size()) {
        //qDebug("Warning: Duplicated items exist in shared string table.");
        //Nothing we can do here, as indices of the strings will be used when loading sheets.
    }

    return true;
}

QT_END_NAMESPACE_XLSX
