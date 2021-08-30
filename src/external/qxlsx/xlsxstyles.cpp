// xlsxstyles.cpp

#include <QtGlobal>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>
#include <QFile>
#include <QMap>
#include <QDataStream>
#include <QDebug>
#include <QBuffer>

#include "xlsxglobal.h"
#include "xlsxstyles_p.h"
#include "xlsxformat_p.h"
#include "xlsxutility_p.h"
#include "xlsxcolor_p.h"

QT_BEGIN_NAMESPACE_XLSX

/*
  When loading from existing .xlsx file. we should create a clean styles object.
  otherwise, default formats should be added.

*/
Styles::Styles(CreateFlag flag)
    : AbstractOOXmlFile(flag), m_nextCustomNumFmtId(176), m_isIndexedColorsDefault(true)
    , m_emptyFormatAdded(false)
{
    //!Fix me. Should the custom num fmt Id starts with 164 or 176 or others??

    //!Fix me! Where should we put these register code?

    // issue #172, #89
#if QT_VERSION >= 0x060000 // Qt 6.0 or over
    if (QMetaType::fromName("XlsxColor").isRegistered())
#else
    #if QT_VERSION >= QT_VERSION_CHECK( 5, 0, 0 ) // Qt 5 or higher
        if (QMetaType::type("XlsxColor") == QMetaType::UnknownType)
    #else
        if (QMetaType::type("XlsxColor") == 0
            || !QMetaType::isRegistered(QMetaType::type("XlsxColor")))
    #endif
#endif
    {
        qRegisterMetaType<XlsxColor>("XlsxColor");

#if QT_VERSION >= 0x060000
        // Qt 6

        ///TODO:

#else
        // Qt 5

        qRegisterMetaTypeStreamOperators<XlsxColor>("XlsxColor");

    #if QT_VERSION >= 0x050200 // 5.2 or higher
            QMetaType::registerDebugStreamOperator<XlsxColor>();
    #endif

#endif
    }

    if (flag == F_NewFromScratch) {
        //Add default Format
        Format defaultFmt;
        addXfFormat(defaultFmt);

        //Add another fill format
        Format fillFmt;
        fillFmt.setFillPattern(Format::PatternGray125);
        m_fillsList.append(fillFmt);
        m_fillsHash.insert(fillFmt.fillKey(), fillFmt);
    }
}

Styles::~Styles()
{
}

Format Styles::xfFormat(int idx) const
{
    if (idx <0 || idx >= m_xf_formatsList.size())
        return Format();

    return m_xf_formatsList[idx];
}

Format Styles::dxfFormat(int idx) const
{
    if (idx <0 || idx >= m_dxf_formatsList.size())
        return Format();

    return m_dxf_formatsList[idx];
}

// dev74 issue#57
void Styles::fixNumFmt(const Format &format)
{
    if (!format.hasNumFmtData())
        return;

    if (format.hasProperty(FormatPrivate::P_NumFmt_Id)
            && !format.stringProperty(FormatPrivate::P_NumFmt_FormatCode).isEmpty())
    {
        return;
    }

    if ( m_builtinNumFmtsHash.isEmpty() )
    {
        m_builtinNumFmtsHash.insert(QStringLiteral("General"), 0);
        m_builtinNumFmtsHash.insert(QStringLiteral("0"), 1);
        m_builtinNumFmtsHash.insert(QStringLiteral("0.00"), 2);
        m_builtinNumFmtsHash.insert(QStringLiteral("#,##0"), 3);
        m_builtinNumFmtsHash.insert(QStringLiteral("#,##0.00"), 4);
//            m_builtinNumFmtsHash.insert(QStringLiteral("($#,##0_);($#,##0)"), 5);
//            m_builtinNumFmtsHash.insert(QStringLiteral("($#,##0_);[Red]($#,##0)"), 6);
//            m_builtinNumFmtsHash.insert(QStringLiteral("($#,##0.00_);($#,##0.00)"), 7);
//            m_builtinNumFmtsHash.insert(QStringLiteral("($#,##0.00_);[Red]($#,##0.00)"), 8);
        m_builtinNumFmtsHash.insert(QStringLiteral("0%"), 9);
        m_builtinNumFmtsHash.insert(QStringLiteral("0.00%"), 10);
        m_builtinNumFmtsHash.insert(QStringLiteral("0.00E+00"), 11);
        m_builtinNumFmtsHash.insert(QStringLiteral("# ?/?"), 12);
        m_builtinNumFmtsHash.insert(QStringLiteral("# ?\?/??"), 13);// Note: "??/" is a c++ trigraph, so escape one "?"
        m_builtinNumFmtsHash.insert(QStringLiteral("m/d/yy"), 14);
        m_builtinNumFmtsHash.insert(QStringLiteral("d-mmm-yy"), 15);
        m_builtinNumFmtsHash.insert(QStringLiteral("d-mmm"), 16);
        m_builtinNumFmtsHash.insert(QStringLiteral("mmm-yy"), 17);
        m_builtinNumFmtsHash.insert(QStringLiteral("h:mm AM/PM"), 18);
        m_builtinNumFmtsHash.insert(QStringLiteral("h:mm:ss AM/PM"), 19);
        m_builtinNumFmtsHash.insert(QStringLiteral("h:mm"), 20);
        m_builtinNumFmtsHash.insert(QStringLiteral("h:mm:ss"), 21);
        m_builtinNumFmtsHash.insert(QStringLiteral("m/d/yy h:mm"), 22);

        m_builtinNumFmtsHash.insert(QStringLiteral("(#,##0_);(#,##0)"), 37);
        m_builtinNumFmtsHash.insert(QStringLiteral("(#,##0_);[Red](#,##0)"), 38);
        m_builtinNumFmtsHash.insert(QStringLiteral("(#,##0.00_);(#,##0.00)"), 39);
        m_builtinNumFmtsHash.insert(QStringLiteral("(#,##0.00_);[Red](#,##0.00)"), 40);
//            m_builtinNumFmtsHash.insert(QStringLiteral("_(* #,##0_);_(* (#,##0);_(* \"-\"_);_(_)"), 41);
//            m_builtinNumFmtsHash.insert(QStringLiteral("_($* #,##0_);_($* (#,##0);_($* \"-\"_);_(_)"), 42);
//            m_builtinNumFmtsHash.insert(QStringLiteral("_(* #,##0.00_);_(* (#,##0.00);_(* \"-\"??_);_(_)"), 43);
//            m_builtinNumFmtsHash.insert(QStringLiteral("_($* #,##0.00_);_($* (#,##0.00);_($* \"-\"??_);_(_)"), 44);
        m_builtinNumFmtsHash.insert(QStringLiteral("mm:ss"), 45);
        m_builtinNumFmtsHash.insert(QStringLiteral("[h]:mm:ss"), 46);
        m_builtinNumFmtsHash.insert(QStringLiteral("mm:ss.0"), 47);
        m_builtinNumFmtsHash.insert(QStringLiteral("##0.0E+0"), 48);
        m_builtinNumFmtsHash.insert(QStringLiteral("@"), 49);

        // dev74
        // m_builtinNumFmtsHash.insert(QStringLiteral("0.####"), 176);

    }

    const auto& str = format.numberFormat();
    if (!str.isEmpty())
    {
        QHash<QString, QSharedPointer<XlsxFormatNumberData> >::ConstIterator cIt;
        //Assign proper number format index
        const auto& it = m_builtinNumFmtsHash.constFind(str);
        if (it != m_builtinNumFmtsHash.constEnd())
        {
            const_cast<Format *>(&format)->fixNumberFormat(it.value(), str);
        }
        else if ((cIt = m_customNumFmtsHash.constFind(str)) != m_customNumFmtsHash.constEnd())
        {
            const_cast<Format *>(&format)->fixNumberFormat(cIt.value()->formatIndex, str);
        }
        else
        {
            //Assign a new fmt Id.
            const_cast<Format *>(&format)->fixNumberFormat(m_nextCustomNumFmtId, str);

            QSharedPointer<XlsxFormatNumberData> fmt(new XlsxFormatNumberData);
            fmt->formatIndex = m_nextCustomNumFmtId;
            fmt->formatString = str;
            m_customNumFmtIdMap.insert(m_nextCustomNumFmtId, fmt);
            m_customNumFmtsHash.insert(str, fmt);

            m_nextCustomNumFmtId += 1;
        }
    }
    else
    {
        const auto id = format.numberFormatIndex();
        //Assign proper format code, this is needed by dxf format
        const auto& it = m_customNumFmtIdMap.constFind(id);
        if (it != m_customNumFmtIdMap.constEnd())
        {
            const_cast<Format *>(&format)->fixNumberFormat(id, it.value()->formatString);
        }
        else
        {
            bool found = false;
            for ( auto&& it = m_builtinNumFmtsHash.constBegin() ; it != m_builtinNumFmtsHash.constEnd() ; ++it )
            {
                if (it.value() == id)
                {
                    const_cast<Format *>(&format)->fixNumberFormat(id, it.key());
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                //Wrong numFmt
                const_cast<Format *>(&format)->fixNumberFormat(id, QStringLiteral("General"));
            }
        }
    }
}

/*
   Assign index to Font/Fill/Border and Format

   When \a force is true, add the format to the format list, even other format has
   the same key have been in.
   This is useful when reading existing .xlsx files which may contains duplicated formats.
*/
void Styles::addXfFormat(const Format &format, bool force)
{
    if (format.isEmpty())
    {
        //Try do something for empty Format.
        if (m_emptyFormatAdded && !force)
            return;

        m_emptyFormatAdded = true;
    }

    //numFmt
    if (format.hasNumFmtData() &&
            !format.hasProperty(FormatPrivate::P_NumFmt_Id))
    {
        fixNumFmt(format);
    }

    //Font
    const auto& fontIt = m_fontsHash.constFind(format.fontKey());
    if (format.hasFontData() && !format.fontIndexValid())
    {
        //Assign proper font index, if has font data.
        if (fontIt == m_fontsHash.constEnd())
            const_cast<Format *>(&format)->setFontIndex(m_fontsList.size());
        else
            const_cast<Format *>(&format)->setFontIndex(fontIt->fontIndex());
    }
    if (fontIt == m_fontsHash.constEnd())
    {
        //Still a valid font if the format has no fontData. (All font properties are default)
        m_fontsList.append(format);
        m_fontsHash[format.fontKey()] = format;
    }

    //Fill
    const auto& fillIt = m_fillsHash.constFind(format.fillKey());
    if (format.hasFillData() && !format.fillIndexValid()) {
        //Assign proper fill index, if has fill data.
        if (fillIt == m_fillsHash.constEnd())
            const_cast<Format *>(&format)->setFillIndex(m_fillsList.size());
        else
            const_cast<Format *>(&format)->setFillIndex(fillIt->fillIndex());
    }
    if (fillIt == m_fillsHash.constEnd()) {
        //Still a valid fill if the format has no fillData. (All fill properties are default)
        m_fillsList.append(format);
        m_fillsHash[format.fillKey()] = format;
    }

    //Border
    const auto& borderIt = m_bordersHash.constFind(format.borderKey());
    if (format.hasBorderData() && !format.borderIndexValid()) {
        //Assign proper border index, if has border data.
        if (borderIt == m_bordersHash.constEnd())
            const_cast<Format *>(&format)->setBorderIndex(m_bordersList.size());
        else
            const_cast<Format *>(&format)->setBorderIndex(borderIt->borderIndex());
    }
    if (borderIt == m_bordersHash.constEnd()) {
        //Still a valid border if the format has no borderData. (All border properties are default)
        m_bordersList.append(format);
        m_bordersHash[format.borderKey()] = format;
    }

    //Format
    const auto& formatIt = m_xf_formatsHash.constFind(format.formatKey());
    if (!format.isEmpty() && !format.xfIndexValid())
    {
        if (formatIt == m_xf_formatsHash.constEnd())
            const_cast<Format *>(&format)->setXfIndex(m_xf_formatsList.size());
        else
            const_cast<Format *>(&format)->setXfIndex(formatIt->xfIndex());
    }

    if (formatIt == m_xf_formatsHash.constEnd() ||
            force)
    {
        m_xf_formatsList.append(format);
        m_xf_formatsHash[format.formatKey()] = format;
    }
}

void Styles::addDxfFormat(const Format &format, bool force)
{
    //numFmt
    if ( format.hasNumFmtData() )
    {
        fixNumFmt(format);
    }

    const auto& formatIt = m_dxf_formatsHash.constFind(format.formatKey());
    if ( !format.isEmpty() &&
            !format.dxfIndexValid() )
    {
        if (formatIt ==  m_dxf_formatsHash.constEnd() ) // m_xf_formatsHash.constEnd()) // issue #108
        {
            const_cast<Format *>(&format)->setDxfIndex( m_dxf_formatsList.size() );
        }
        else
        {
            const_cast<Format *>(&format)->setDxfIndex( formatIt->dxfIndex() );
        }
    }

    if (formatIt == m_xf_formatsHash.constEnd() ||
         force )
    {
        m_dxf_formatsList.append(format);
        m_dxf_formatsHash[ format.formatKey() ] = format;
    }
}

void Styles::saveToXmlFile(QIODevice *device) const
{
    QXmlStreamWriter writer(device);

    writer.writeStartDocument(QStringLiteral("1.0"), true);
    writer.writeStartElement(QStringLiteral("styleSheet"));
    writer.writeAttribute(QStringLiteral("xmlns"), QStringLiteral("http://schemas.openxmlformats.org/spreadsheetml/2006/main"));

    writeNumFmts(writer);
    writeFonts(writer);
    writeFills(writer);
    writeBorders(writer);

    writer.writeStartElement(QStringLiteral("cellStyleXfs"));
    writer.writeAttribute(QStringLiteral("count"), QStringLiteral("1"));
    writer.writeStartElement(QStringLiteral("xf"));
    writer.writeAttribute(QStringLiteral("numFmtId"), QStringLiteral("0"));
    writer.writeAttribute(QStringLiteral("fontId"), QStringLiteral("0"));
    writer.writeAttribute(QStringLiteral("fillId"), QStringLiteral("0"));
    writer.writeAttribute(QStringLiteral("borderId"), QStringLiteral("0"));
    writer.writeEndElement();//xf
    writer.writeEndElement();//cellStyleXfs

    writeCellXfs(writer);

    writer.writeStartElement(QStringLiteral("cellStyles"));
    writer.writeAttribute(QStringLiteral("count"), QStringLiteral("1"));
    writer.writeStartElement(QStringLiteral("cellStyle"));
    writer.writeAttribute(QStringLiteral("name"), QStringLiteral("Normal"));
    writer.writeAttribute(QStringLiteral("xfId"), QStringLiteral("0"));
    writer.writeAttribute(QStringLiteral("builtinId"), QStringLiteral("0"));
    writer.writeEndElement();//cellStyle
    writer.writeEndElement();//cellStyles

    writeDxfs(writer);

    writer.writeStartElement(QStringLiteral("tableStyles"));
    writer.writeAttribute(QStringLiteral("count"), QStringLiteral("0"));
    writer.writeAttribute(QStringLiteral("defaultTableStyle"), QStringLiteral("TableStyleMedium9"));
    writer.writeAttribute(QStringLiteral("defaultPivotStyle"), QStringLiteral("PivotStyleLight16"));
    writer.writeEndElement();//tableStyles

    writeColors(writer);

    writer.writeEndElement();//styleSheet
    writer.writeEndDocument();
}

void Styles::writeNumFmts(QXmlStreamWriter &writer) const
{
    if (m_customNumFmtIdMap.size() == 0)
        return;

    writer.writeStartElement(QStringLiteral("numFmts"));
    writer.writeAttribute(QStringLiteral("count"), QString::number(m_customNumFmtIdMap.count()));

    QMapIterator<int, QSharedPointer<XlsxFormatNumberData> > it(m_customNumFmtIdMap);
    while (it.hasNext()) {
        it.next();
        writer.writeEmptyElement(QStringLiteral("numFmt"));
        writer.writeAttribute(QStringLiteral("numFmtId"), QString::number(it.value()->formatIndex));
        writer.writeAttribute(QStringLiteral("formatCode"), it.value()->formatString);
    }
    writer.writeEndElement();//numFmts
}

/*
*/
void Styles::writeFonts(QXmlStreamWriter &writer) const
{
    writer.writeStartElement(QStringLiteral("fonts"));
    writer.writeAttribute(QStringLiteral("count"), QString::number(m_fontsList.count()));
    for (size_t i=0; i<m_fontsList.size(); ++i)
        writeFont(writer, m_fontsList[i], false);
    writer.writeEndElement();//fonts
}

void Styles::writeFont(QXmlStreamWriter &writer, const Format &format, bool isDxf) const
{
    writer.writeStartElement(QStringLiteral("font"));

    //The condense and extend elements are mainly used in dxf format
    if (format.hasProperty(FormatPrivate::P_Font_Condense)
            && !format.boolProperty(FormatPrivate::P_Font_Condense)) {
        writer.writeEmptyElement(QStringLiteral("condense"));
        writer.writeAttribute(QStringLiteral("val"), QStringLiteral("0"));
    }
    if (format.hasProperty(FormatPrivate::P_Font_Extend)
            && !format.boolProperty(FormatPrivate::P_Font_Extend)) {
        writer.writeEmptyElement(QStringLiteral("extend"));
        writer.writeAttribute(QStringLiteral("val"), QStringLiteral("0"));
    }

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

    if (!isDxf && format.hasProperty(FormatPrivate::P_Font_Size)) {
        writer.writeEmptyElement(QStringLiteral("sz"));
        writer.writeAttribute(QStringLiteral("val"), QString::number(format.fontSize()));
    }

    if (format.hasProperty(FormatPrivate::P_Font_Color)) {
        XlsxColor color = format.property(FormatPrivate::P_Font_Color).value<XlsxColor>();
        color.saveToXml(writer);
    }

    if (!isDxf) {
        if (!format.fontName().isEmpty()) {
            writer.writeEmptyElement(QStringLiteral("name"));
            writer.writeAttribute(QStringLiteral("val"), format.fontName());
        }
        if (format.hasProperty(FormatPrivate::P_Font_Charset)) {
            writer.writeEmptyElement(QStringLiteral("charset"));
            writer.writeAttribute(QStringLiteral("val"), QString::number(format.intProperty(FormatPrivate::P_Font_Charset)));
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
    writer.writeEndElement(); //font
}

void Styles::writeFills(QXmlStreamWriter &writer) const
{
    writer.writeStartElement(QStringLiteral("fills"));
    writer.writeAttribute(QStringLiteral("count"), QString::number(m_fillsList.size()));

    for (size_t i=0; i<m_fillsList.size(); ++i)
        writeFill(writer, m_fillsList[i]);

    writer.writeEndElement(); //fills
}

void Styles::writeFill(QXmlStreamWriter &writer, const Format &fill, bool isDxf) const
{
    static const QMap<int, QString> patternStrings = {
        {Format::PatternNone, QStringLiteral("none")},
        {Format::PatternSolid, QStringLiteral("solid")},
        {Format::PatternMediumGray, QStringLiteral("mediumGray")},
        {Format::PatternDarkGray, QStringLiteral("darkGray")},
        {Format::PatternLightGray, QStringLiteral("lightGray")},
        {Format::PatternDarkHorizontal, QStringLiteral("darkHorizontal")},
        {Format::PatternDarkVertical, QStringLiteral("darkVertical")},
        {Format::PatternDarkDown, QStringLiteral("darkDown")},
        {Format::PatternDarkUp, QStringLiteral("darkUp")},
        {Format::PatternDarkGrid, QStringLiteral("darkGrid")},
        {Format::PatternDarkTrellis, QStringLiteral("darkTrellis")},
        {Format::PatternLightHorizontal, QStringLiteral("lightHorizontal")},
        {Format::PatternLightVertical, QStringLiteral("lightVertical")},
        {Format::PatternLightDown, QStringLiteral("lightDown")},
        {Format::PatternLightUp, QStringLiteral("lightUp")},
        {Format::PatternLightTrellis, QStringLiteral("lightTrellis")},
        {Format::PatternGray125, QStringLiteral("gray125")},
        {Format::PatternGray0625, QStringLiteral("gray0625")},
        {Format::PatternLightGrid, QStringLiteral("lightGrid")}
    };

    writer.writeStartElement(QStringLiteral("fill"));
    writer.writeStartElement(QStringLiteral("patternFill"));
    Format::FillPattern pattern = fill.fillPattern();
    // For normal fill formats, Excel prefer to outputing the default "none" attribute
    // But for dxf, Excel prefer to omiting the default "none"
    // Though not make any difference, but it make easier to compare origin files with generate files during debug
    if (!(pattern == Format::PatternNone && isDxf))
        writer.writeAttribute(QStringLiteral("patternType"), patternStrings[pattern]);
    // For a solid fill, Excel reverses the role of foreground and background colours
    if (fill.fillPattern() == Format::PatternSolid) {
        if (fill.hasProperty(FormatPrivate::P_Fill_BgColor))
            fill.property(FormatPrivate::P_Fill_BgColor).value<XlsxColor>().saveToXml(writer, QStringLiteral("fgColor"));
        if (fill.hasProperty(FormatPrivate::P_Fill_FgColor))
            fill.property(FormatPrivate::P_Fill_FgColor).value<XlsxColor>().saveToXml(writer, QStringLiteral("bgColor"));
    } else {
        if (fill.hasProperty(FormatPrivate::P_Fill_FgColor))
            fill.property(FormatPrivate::P_Fill_FgColor).value<XlsxColor>().saveToXml(writer, QStringLiteral("fgColor"));
        if (fill.hasProperty(FormatPrivate::P_Fill_BgColor))
            fill.property(FormatPrivate::P_Fill_BgColor).value<XlsxColor>().saveToXml(writer, QStringLiteral("bgColor"));
    }
    writer.writeEndElement();//patternFill
    writer.writeEndElement();//fill
}

void Styles::writeBorders(QXmlStreamWriter &writer) const
{
    writer.writeStartElement(QStringLiteral("borders"));
    writer.writeAttribute(QStringLiteral("count"), QString::number(m_bordersList.count()));
    for (size_t i=0; i<m_bordersList.size(); ++i)
        writeBorder(writer, m_bordersList[i]);
    writer.writeEndElement();//borders
}

void Styles::writeBorder(QXmlStreamWriter &writer, const Format &border, bool isDxf) const
{
    writer.writeStartElement(QStringLiteral("border"));
    if (border.hasProperty(FormatPrivate::P_Border_DiagonalType)) {
        Format::DiagonalBorderType t = border.diagonalBorderType();
        if (t == Format::DiagonalBorderUp) {
            writer.writeAttribute(QStringLiteral("diagonalUp"), QStringLiteral("1"));
        } else if (t == Format::DiagonalBorderDown) {
            writer.writeAttribute(QStringLiteral("diagonalDown"), QStringLiteral("1"));
        } else if (t == Format::DiagnoalBorderBoth) {
            writer.writeAttribute(QStringLiteral("diagonalUp"), QStringLiteral("1"));
            writer.writeAttribute(QStringLiteral("diagonalDown"), QStringLiteral("1"));
        }
    }

    writeSubBorder(writer, QStringLiteral("left"), border.leftBorderStyle(), border.property(FormatPrivate::P_Border_LeftColor).value<XlsxColor>());
    writeSubBorder(writer, QStringLiteral("right"), border.rightBorderStyle(), border.property(FormatPrivate::P_Border_RightColor).value<XlsxColor>());
    writeSubBorder(writer, QStringLiteral("top"), border.topBorderStyle(), border.property(FormatPrivate::P_Border_TopColor).value<XlsxColor>());
    writeSubBorder(writer, QStringLiteral("bottom"), border.bottomBorderStyle(), border.property(FormatPrivate::P_Border_BottomColor).value<XlsxColor>());

    //Condition DXF formats don't allow diagonal style
    if (!isDxf)
        writeSubBorder(writer, QStringLiteral("diagonal"), border.diagonalBorderStyle(), border.property(FormatPrivate::P_Border_DiagonalColor).value<XlsxColor>());

    if (isDxf) {
//        writeSubBorder(wirter, QStringLiteral("vertical"), );
//        writeSubBorder(writer, QStringLiteral("horizontal"), );
    }

    writer.writeEndElement();//border
}

void Styles::writeSubBorder(QXmlStreamWriter &writer, const QString &type, int style, const XlsxColor &color) const
{
    if (style == Format::BorderNone) {
        writer.writeEmptyElement(type);
        return;
    }

    static const QMap<int, QString> stylesString = {
        {Format::BorderNone, QStringLiteral("none")},
        {Format::BorderThin, QStringLiteral("thin")},
        {Format::BorderMedium, QStringLiteral("medium")},
        {Format::BorderDashed, QStringLiteral("dashed")},
        {Format::BorderDotted, QStringLiteral("dotted")},
        {Format::BorderThick, QStringLiteral("thick")},
        {Format::BorderDouble, QStringLiteral("double")},
        {Format::BorderHair, QStringLiteral("hair")},
        {Format::BorderMediumDashed, QStringLiteral("mediumDashed")},
        {Format::BorderDashDot, QStringLiteral("dashDot")},
        {Format::BorderMediumDashDot, QStringLiteral("mediumDashDot")},
        {Format::BorderDashDotDot, QStringLiteral("dashDotDot")},
        {Format::BorderMediumDashDotDot, QStringLiteral("mediumDashDotDot")},
        {Format::BorderSlantDashDot, QStringLiteral("slantDashDot")}
    };

    writer.writeStartElement(type);
    writer.writeAttribute(QStringLiteral("style"), stylesString[style]);
    color.saveToXml(writer); //write color element

    writer.writeEndElement();//type
}

void Styles::writeCellXfs(QXmlStreamWriter &writer) const
{
    writer.writeStartElement(QStringLiteral("cellXfs"));
    writer.writeAttribute(QStringLiteral("count"), QString::number(m_xf_formatsList.size()));
    for (const Format &format : m_xf_formatsList) {
        int xf_id = 0;
        writer.writeStartElement(QStringLiteral("xf"));
        writer.writeAttribute(QStringLiteral("numFmtId"), QString::number(format.numberFormatIndex()));
        writer.writeAttribute(QStringLiteral("fontId"), QString::number(format.fontIndex()));
        writer.writeAttribute(QStringLiteral("fillId"), QString::number(format.fillIndex()));
        writer.writeAttribute(QStringLiteral("borderId"), QString::number(format.borderIndex()));
        writer.writeAttribute(QStringLiteral("xfId"), QString::number(xf_id));
        if (format.hasNumFmtData())
            writer.writeAttribute(QStringLiteral("applyNumberFormat"), QStringLiteral("1"));
        if (format.hasFontData())
            writer.writeAttribute(QStringLiteral("applyFont"), QStringLiteral("1"));
        if (format.hasFillData())
            writer.writeAttribute(QStringLiteral("applyFill"), QStringLiteral("1"));
        if (format.hasBorderData())
            writer.writeAttribute(QStringLiteral("applyBorder"), QStringLiteral("1"));
        if (format.hasAlignmentData())
            writer.writeAttribute(QStringLiteral("applyAlignment"), QStringLiteral("1"));

        if (format.hasAlignmentData()) {
            writer.writeEmptyElement(QStringLiteral("alignment"));
            if (format.hasProperty(FormatPrivate::P_Alignment_AlignH)) {
                switch (format.horizontalAlignment()) {
                case Format::AlignLeft:
                    writer.writeAttribute(QStringLiteral("horizontal"), QStringLiteral("left"));
                    break;
                case Format::AlignHCenter:
                    writer.writeAttribute(QStringLiteral("horizontal"), QStringLiteral("center"));
                    break;
                case Format::AlignRight:
                    writer.writeAttribute(QStringLiteral("horizontal"), QStringLiteral("right"));
                    break;
                case Format::AlignHFill:
                    writer.writeAttribute(QStringLiteral("horizontal"), QStringLiteral("fill"));
                    break;
                case Format::AlignHJustify:
                    writer.writeAttribute(QStringLiteral("horizontal"), QStringLiteral("justify"));
                    break;
                case Format::AlignHMerge:
                    writer.writeAttribute(QStringLiteral("horizontal"), QStringLiteral("centerContinuous"));
                    break;
                case Format::AlignHDistributed:
                    writer.writeAttribute(QStringLiteral("horizontal"), QStringLiteral("distributed"));
                    break;
                default:
                    break;
                }
            }

            if (format.hasProperty(FormatPrivate::P_Alignment_AlignV)) {
                switch (format.verticalAlignment()) {
                case Format::AlignTop:
                    writer.writeAttribute(QStringLiteral("vertical"), QStringLiteral("top"));
                    break;
                case Format::AlignVCenter:
                    writer.writeAttribute(QStringLiteral("vertical"), QStringLiteral("center"));
                    break;
                case Format::AlignVJustify:
                    writer.writeAttribute(QStringLiteral("vertical"), QStringLiteral("justify"));
                    break;
                case Format::AlignVDistributed:
                    writer.writeAttribute(QStringLiteral("vertical"), QStringLiteral("distributed"));
                    break;
                default:
                    break;
                }
            }
            if (format.hasProperty(FormatPrivate::P_Alignment_Indent))
                writer.writeAttribute(QStringLiteral("indent"), QString::number(format.indent()));
            if (format.hasProperty(FormatPrivate::P_Alignment_Wrap) && format.textWrap())
                writer.writeAttribute(QStringLiteral("wrapText"), QStringLiteral("1"));
            if (format.hasProperty(FormatPrivate::P_Alignment_ShinkToFit) && format.shrinkToFit())
                writer.writeAttribute(QStringLiteral("shrinkToFit"), QStringLiteral("1"));
            if (format.hasProperty(FormatPrivate::P_Alignment_Rotation))
                writer.writeAttribute(QStringLiteral("textRotation"), QString::number(format.rotation()));
        }

        writer.writeEndElement();//xf
    }
    writer.writeEndElement();//cellXfs
}

void Styles::writeDxfs(QXmlStreamWriter &writer) const
{
    writer.writeStartElement(QStringLiteral("dxfs"));
    writer.writeAttribute(QStringLiteral("count"), QString::number(m_dxf_formatsList.size()));
    for (const Format &format : m_dxf_formatsList)
        writeDxf(writer, format);
    writer.writeEndElement(); //dxfs
}

void Styles::writeDxf(QXmlStreamWriter &writer, const Format &format) const
{
    writer.writeStartElement(QStringLiteral("dxf"));

    if (format.hasFontData())
        writeFont(writer, format, true);

    if (format.hasNumFmtData()) {
        writer.writeEmptyElement(QStringLiteral("numFmt"));
        writer.writeAttribute(QStringLiteral("numFmtId"), QString::number(format.numberFormatIndex()));
        writer.writeAttribute(QStringLiteral("formatCode"), format.numberFormat());
    }

    if (format.hasFillData())
        writeFill(writer, format, true);

    if (format.hasBorderData())
        writeBorder(writer, format, true);

    writer.writeEndElement();//dxf
}

void Styles::writeColors(QXmlStreamWriter &writer) const
{
    if (m_isIndexedColorsDefault) //Don't output the default indexdeColors
        return;

    writer.writeStartElement(QStringLiteral("colors"));

    writer.writeStartElement(QStringLiteral("indexedColors"));
    for (const QColor &color : m_indexedColors) {
        writer.writeEmptyElement(QStringLiteral("rgbColor"));
        writer.writeAttribute(QStringLiteral("rgb"), XlsxColor::toARGBString(color));
    }

    writer.writeEndElement();//indexedColors

    writer.writeEndElement();//colors
}

bool Styles::readNumFmts(QXmlStreamReader &reader)
{
    Q_ASSERT(reader.name() == QLatin1String("numFmts"));
    const auto& attributes = reader.attributes();
    const auto hasCount = attributes.hasAttribute(QLatin1String("count"));
    const auto count = hasCount ? attributes.value(QLatin1String("count")).toString().toInt() : -1;

    //Read utill we find the numFmts end tag or ....
    while (!reader.atEnd() && !(reader.tokenType() == QXmlStreamReader::EndElement
           && reader.name() == QLatin1String("numFmts"))) {
        reader.readNextStartElement();
        if (reader.tokenType() == QXmlStreamReader::StartElement) {
            if (reader.name() == QLatin1String("numFmt")) {
                const auto& attributes = reader.attributes();
                QSharedPointer<XlsxFormatNumberData> fmt (new XlsxFormatNumberData);
                fmt->formatIndex = attributes.value(QLatin1String("numFmtId")).toString().toInt();
                fmt->formatString = attributes.value(QLatin1String("formatCode")).toString();
                if (fmt->formatIndex >= m_nextCustomNumFmtId)
                    m_nextCustomNumFmtId = fmt->formatIndex + 1;
                m_customNumFmtIdMap.insert(fmt->formatIndex, fmt);
                m_customNumFmtsHash.insert(fmt->formatString, fmt);
            }
        }
    }

    if (reader.hasError())
        qWarning()<<reader.errorString();

    if (hasCount && (count != m_customNumFmtIdMap.size()))
        qWarning("error read custom numFmts");

    return true;
}

bool Styles::readFonts(QXmlStreamReader &reader)
{
    Q_ASSERT(reader.name() == QLatin1String("fonts"));
    const auto& attributes = reader.attributes();
    const auto hasCount = attributes.hasAttribute(QLatin1String("count"));
    const auto count = hasCount ? attributes.value(QLatin1String("count")).toString().toInt() : -1;
    while (!reader.atEnd() && !(reader.tokenType() == QXmlStreamReader::EndElement
                               && reader.name() == QLatin1String("fonts"))) {
        reader.readNextStartElement();
        if (reader.tokenType() == QXmlStreamReader::StartElement) {
            if (reader.name() == QLatin1String("font")) {
                Format format;
                readFont(reader, format);
                m_fontsList.append(format);
                m_fontsHash.insert(format.fontKey(), format);
                if (format.isValid())
                    format.setFontIndex(m_fontsList.size()-1);
            }
        }
    }
    if (reader.hasError())
        qWarning()<<reader.errorString();

    if (hasCount && (count != m_fontsList.size()))
        qWarning("error read fonts");
    return true;
}

bool Styles::readFont(QXmlStreamReader &reader, Format &format)
{
    Q_ASSERT(reader.name() == QLatin1String("font"));
    while (!reader.atEnd() && !(reader.tokenType() == QXmlStreamReader::EndElement
                               && reader.name() == QLatin1String("font"))) {
        reader.readNextStartElement();
        if (reader.tokenType() == QXmlStreamReader::StartElement) {
            const auto& attributes = reader.attributes();
            if (reader.name() == QLatin1String("name")) {
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
                const auto sz = attributes.value(QLatin1String("val")).toString().toInt();
                format.setFontSize(sz);
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
    return true;
}

bool Styles::readFills(QXmlStreamReader &reader)
{
    Q_ASSERT(reader.name() == QLatin1String("fills"));

    const auto& attributes = reader.attributes();
    const auto hasCount = attributes.hasAttribute(QLatin1String("count"));
    const auto count = hasCount ? attributes.value(QLatin1String("count")).toString().toInt() : -1;
    while (!reader.atEnd() && !(reader.tokenType() == QXmlStreamReader::EndElement
                               && reader.name() == QLatin1String("fills"))) {
        reader.readNextStartElement();
        if (reader.tokenType() == QXmlStreamReader::StartElement) {
            if (reader.name() == QLatin1String("fill")) {
                Format fill;
                readFill(reader, fill);
                m_fillsList.append(fill);
                m_fillsHash.insert(fill.fillKey(), fill);
                if (fill.isValid())
                    fill.setFillIndex(m_fillsList.size()-1);
            }
        }
    }
    if (reader.hasError())
        qWarning()<<reader.errorString();

    if (hasCount && (count != m_fillsList.size()))
        qWarning("error read fills");
    return true;
}

bool Styles::readFill(QXmlStreamReader &reader, Format &fill)
{
    Q_ASSERT(reader.name() == QLatin1String("fill"));

    static const QMap<QString, Format::FillPattern> patternValues = {
        {QStringLiteral("none"), Format::PatternNone},
        {QStringLiteral("solid"), Format::PatternSolid},
        {QStringLiteral("mediumGray"), Format::PatternMediumGray},
        {QStringLiteral("darkGray"), Format::PatternDarkGray},
        {QStringLiteral("lightGray"), Format::PatternLightGray},
        {QStringLiteral("darkHorizontal"), Format::PatternDarkHorizontal},
        {QStringLiteral("darkVertical"), Format::PatternDarkVertical},
        {QStringLiteral("darkDown"), Format::PatternDarkDown},
        {QStringLiteral("darkUp"), Format::PatternDarkUp},
        {QStringLiteral("darkGrid"), Format::PatternDarkGrid},
        {QStringLiteral("darkTrellis"), Format::PatternDarkTrellis},
        {QStringLiteral("lightHorizontal"), Format::PatternLightHorizontal},
        {QStringLiteral("lightVertical"), Format::PatternLightVertical},
        {QStringLiteral("lightDown"), Format::PatternLightDown},
        {QStringLiteral("lightUp"), Format::PatternLightUp},
        {QStringLiteral("lightTrellis"), Format::PatternLightTrellis},
        {QStringLiteral("gray125"), Format::PatternGray125},
        {QStringLiteral("gray0625"), Format::PatternGray0625},
        {QStringLiteral("lightGrid"), Format::PatternLightGrid}
    };

    while (!reader.atEnd() && !(reader.tokenType() == QXmlStreamReader::EndElement && reader.name() == QLatin1String("fill"))) {
        reader.readNextStartElement();
        if (reader.tokenType() == QXmlStreamReader::StartElement) {
            if (reader.name() == QLatin1String("patternFill")) {
                const auto& attributes = reader.attributes();
                if (attributes.hasAttribute(QLatin1String("patternType"))) {
                    const auto& it = patternValues.constFind(attributes.value(QLatin1String("patternType")).toString());
                    fill.setFillPattern(it != patternValues.constEnd() ? it.value() : Format::PatternNone);

                    //parse foreground and background colors if they exist
                    while (!reader.atEnd() && !(reader.tokenType() == QXmlStreamReader::EndElement && reader.name() == QLatin1String("patternFill"))) {
                        reader.readNextStartElement();
                        if (reader.tokenType() == QXmlStreamReader::StartElement) {
                            if (reader.name() == QLatin1String("fgColor")) {
                                XlsxColor c;
                                if ( c.loadFromXml(reader) )
                                {
                                    if (fill.fillPattern() == Format::PatternSolid)
                                        fill.setProperty(FormatPrivate::P_Fill_BgColor, c);
                                    else
                                        fill.setProperty(FormatPrivate::P_Fill_FgColor, c);
                                }
                            } else if (reader.name() == QLatin1String("bgColor")) {
                                XlsxColor c;
                                if ( c.loadFromXml(reader) )
                                {
                                    if (fill.fillPattern() == Format::PatternSolid)
                                        fill.setProperty(FormatPrivate::P_Fill_FgColor, c);
                                    else
                                        fill.setProperty(FormatPrivate::P_Fill_BgColor, c);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return true;
}

bool Styles::readBorders(QXmlStreamReader &reader)
{
    Q_ASSERT(reader.name() == QLatin1String("borders"));

    const auto& attributes = reader.attributes();
    const auto hasCount = attributes.hasAttribute(QLatin1String("count"));
    const auto count = hasCount ? attributes.value(QLatin1String("count")).toString().toInt() : -1;
    while (!reader.atEnd() && !(reader.tokenType() == QXmlStreamReader::EndElement
                               && reader.name() == QLatin1String("borders"))) {
        reader.readNextStartElement();
        if (reader.tokenType() == QXmlStreamReader::StartElement) {
            if (reader.name() == QLatin1String("border")) {
                Format border;
                readBorder(reader, border);
                m_bordersList.append(border);
                m_bordersHash.insert(border.borderKey(), border);
                if (border.isValid())
                    border.setBorderIndex(m_bordersList.size()-1);
            }
        }
    }

    if (reader.hasError())
        qWarning()<<reader.errorString();

    if (hasCount && (count != m_bordersList.size()))
        qWarning("error read borders");

    return true;
}

bool Styles::readBorder(QXmlStreamReader &reader, Format &border)
{
    Q_ASSERT(reader.name() == QLatin1String("border"));

    const auto& attributes = reader.attributes();
    const auto isUp = attributes.hasAttribute(QLatin1String("diagonalUp"));
    const auto isDown = attributes.hasAttribute(QLatin1String("diagonalUp"));
    if (isUp && isDown)
        border.setDiagonalBorderType(Format::DiagnoalBorderBoth);
    else if (isUp)
        border.setDiagonalBorderType(Format::DiagonalBorderUp);
    else if (isDown)
        border.setDiagonalBorderType(Format::DiagonalBorderDown);

    while (!reader.atEnd() && !(reader.tokenType() == QXmlStreamReader::EndElement && reader.name() == QLatin1String("border"))) {
        reader.readNextStartElement();
        if (reader.tokenType() == QXmlStreamReader::StartElement) {
            if (reader.name() == QLatin1String("left") || reader.name() == QLatin1String("right")
                    || reader.name() == QLatin1String("top") || reader.name() == QLatin1String("bottom")
                    || reader.name() == QLatin1String("diagonal") ) {
                Format::BorderStyle style(Format::BorderNone);
                XlsxColor color;
                readSubBorder(reader, reader.name().toString(), style, color);

                if (reader.name() == QLatin1String("left")) {
                    border.setLeftBorderStyle(style);
                    if (!color.isInvalid())
                        border.setProperty(FormatPrivate::P_Border_LeftColor, color);
                } else if (reader.name() == QLatin1String("right")) {
                    border.setRightBorderStyle(style);
                    if (!color.isInvalid())
                        border.setProperty(FormatPrivate::P_Border_RightColor, color);
                } else if (reader.name() == QLatin1String("top")) {
                    border.setTopBorderStyle(style);
                    if (!color.isInvalid())
                        border.setProperty(FormatPrivate::P_Border_TopColor, color);
                } else if (reader.name() == QLatin1String("bottom")) {
                    border.setBottomBorderStyle(style);
                    if (!color.isInvalid())
                        border.setProperty(FormatPrivate::P_Border_BottomColor, color);
                } else if (reader.name() == QLatin1String("diagonal")) {
                    border.setDiagonalBorderStyle(style);
                    if (!color.isInvalid())
                        border.setProperty(FormatPrivate::P_Border_DiagonalColor, color);
                }
            }
        }

        if (reader.tokenType() == QXmlStreamReader::EndElement && reader.name() == QLatin1String("border"))
            break;
    }

    return true;
}

bool Styles::readCellStyleXfs(QXmlStreamReader &reader)
{
    Q_UNUSED(reader);
    return true;
}

bool Styles::readSubBorder(QXmlStreamReader &reader, const QString &name, Format::BorderStyle &style, XlsxColor &color)
{
    Q_ASSERT(reader.name() == name);

    static const QMap<QString, Format::BorderStyle> stylesStringsMap = {
        {QStringLiteral("none"), Format::BorderNone},
        {QStringLiteral("thin"), Format::BorderThin},
        {QStringLiteral("medium"), Format::BorderMedium},
        {QStringLiteral("dashed"), Format::BorderDashed},
        {QStringLiteral("dotted"), Format::BorderDotted},
        {QStringLiteral("thick"), Format::BorderThick},
        {QStringLiteral("double"), Format::BorderDouble},
        {QStringLiteral("hair"), Format::BorderHair},
        {QStringLiteral("mediumDashed"), Format::BorderMediumDashed},
        {QStringLiteral("dashDot"), Format::BorderDashDot},
        {QStringLiteral("mediumDashDot"), Format::BorderMediumDashDot},
        {QStringLiteral("dashDotDot"), Format::BorderDashDotDot},
        {QStringLiteral("mediumDashDotDot"), Format::BorderMediumDashDotDot},
        {QStringLiteral("slantDashDot"), Format::BorderSlantDashDot}
    };

    const auto& attributes = reader.attributes();
    if (attributes.hasAttribute(QLatin1String("style"))) {
        QString styleString = attributes.value(QLatin1String("style")).toString();
        const auto& it = stylesStringsMap.constFind(styleString);
        if (it != stylesStringsMap.constEnd()) {
            //get style
            style = it.value();
            while (!reader.atEnd() && !(reader.tokenType() == QXmlStreamReader::EndElement && reader.name() == name)) {
                reader.readNextStartElement();
                if (reader.tokenType() == QXmlStreamReader::StartElement) {
                    if (reader.name() == QLatin1String("color"))
                        color.loadFromXml(reader);
                }
            }
        }
    }

    return true;
}

bool Styles::readCellXfs(QXmlStreamReader &reader)
{
    Q_ASSERT(reader.name() == QLatin1String("cellXfs"));
    const auto& attributes = reader.attributes();
    const auto hasCount = attributes.hasAttribute(QLatin1String("count"));
    const auto count = hasCount ? attributes.value(QLatin1String("count")).toString().toInt() : -1;
    while (!reader.atEnd() && !(reader.tokenType() == QXmlStreamReader::EndElement
                                && reader.name() == QLatin1String("cellXfs"))) {
        reader.readNextStartElement();
        if (reader.tokenType() == QXmlStreamReader::StartElement) {
            if (reader.name() == QLatin1String("xf")) {

                Format format;
                const auto& xfAttrs = reader.attributes();

                //        qDebug()<<reader.name()<<reader.tokenString()<<" .........";
                //        for (int i=0; i<xfAttrs.size(); ++i)
                //            qDebug()<<"... "<<i<<" "<<xfAttrs[i].name()<<xfAttrs[i].value();

                if (xfAttrs.hasAttribute(QLatin1String("numFmtId"))) {
                    const auto numFmtIndex = xfAttrs.value(QLatin1String("numFmtId")).toString().toInt();
                    const auto apply = parseXsdBoolean(xfAttrs.value(QLatin1String("applyNumberFormat")).toString());
                    if(apply) {
                        const auto& it = m_customNumFmtIdMap.constFind(numFmtIndex);
                        if (it == m_customNumFmtIdMap.constEnd())
                            format.setNumberFormatIndex(numFmtIndex);
                        else
                            format.setNumberFormat(numFmtIndex, it.value()->formatString);
                    }
                }

                if (xfAttrs.hasAttribute(QLatin1String("fontId"))) {
                    const auto fontIndex = xfAttrs.value(QLatin1String("fontId")).toString().toInt();
                    if (fontIndex >= m_fontsList.size()) {
                        qDebug("Error read styles.xml, cellXfs fontId");
                    } else {
                        const auto apply = parseXsdBoolean(xfAttrs.value(QLatin1String("applyFont")).toString());
                        if(apply) {
                            Format fontFormat = m_fontsList[fontIndex];
                            for (int i=FormatPrivate::P_Font_STARTID; i<FormatPrivate::P_Font_ENDID; ++i) {
                                if (fontFormat.hasProperty(i))
                                    format.setProperty(i, fontFormat.property(i));
                            }
                        }
                    }
                }

                if (xfAttrs.hasAttribute(QLatin1String("fillId"))) {
                    const auto id = xfAttrs.value(QLatin1String("fillId")).toString().toInt();
                    if (id >= m_fillsList.size()) {
                        qDebug("Error read styles.xml, cellXfs fillId");
                    } else {

                        // dev20 branch
                        // NOTE: MIcrosoft Excel does not have 'applyFill' tag.
                        //

                        // bool apply = parseXsdBoolean(xfAttrs.value(QLatin1String("applyFill")).toString());
                        // if (apply)

                        {
                            Format fillFormat = m_fillsList[id];
                            for (int i=FormatPrivate::P_Fill_STARTID; i<FormatPrivate::P_Fill_ENDID; ++i)
                            {
                                if (fillFormat.hasProperty(i))
                                    format.setProperty(i, fillFormat.property(i));
                            }
                        }

                    }
                }

                if (xfAttrs.hasAttribute(QLatin1String("borderId"))) {
                    const auto id = xfAttrs.value(QLatin1String("borderId")).toString().toInt();
                    if (id >= m_bordersList.size()) {
                        qDebug("Error read styles.xml, cellXfs borderId");
                    } else {
                        const auto apply = parseXsdBoolean(xfAttrs.value(QLatin1String("applyBorder")).toString());
                        if(apply) {
                            Format borderFormat = m_bordersList[id];
                            for (int i=FormatPrivate::P_Border_STARTID; i<FormatPrivate::P_Border_ENDID; ++i) {
                                if (borderFormat.hasProperty(i))
                                    format.setProperty(i, borderFormat.property(i));
                            }
                        }
                    }
                }

                const auto apply = parseXsdBoolean(xfAttrs.value(QLatin1String("applyAlignment")).toString());
                if(apply) {
                    reader.readNextStartElement();
                    if (reader.name() == QLatin1String("alignment")) {
                        const auto& alignAttrs = reader.attributes();

                        if (alignAttrs.hasAttribute(QLatin1String("horizontal"))) {
                            static const QMap<QString, Format::HorizontalAlignment> alignStringMap = {
                                {QStringLiteral("left"), Format::AlignLeft},
                                {QStringLiteral("center"), Format::AlignHCenter},
                                {QStringLiteral("right"), Format::AlignRight},
                                {QStringLiteral("justify"), Format::AlignHJustify},
                                {QStringLiteral("centerContinuous"), Format::AlignHMerge},
                                {QStringLiteral("distributed"), Format::AlignHDistributed}
                            };

                            const auto& it = alignStringMap.constFind(alignAttrs.value(QLatin1String("horizontal")).toString());
                            if (it != alignStringMap.constEnd())
                                format.setHorizontalAlignment(it.value());
                        }

                        if (alignAttrs.hasAttribute(QLatin1String("vertical"))) {
                            static const QMap<QString, Format::VerticalAlignment> alignStringMap = {
                                {QStringLiteral("top"), Format::AlignTop},
                                {QStringLiteral("center"), Format::AlignVCenter},
                                {QStringLiteral("justify"), Format::AlignVJustify},
                                {QStringLiteral("distributed"), Format::AlignVDistributed}
                            };

                            const auto& it = alignStringMap.constFind(alignAttrs.value(QLatin1String("vertical")).toString());
                            if (it != alignStringMap.constEnd())
                                format.setVerticalAlignment(it.value());
                        }

                        if (alignAttrs.hasAttribute(QLatin1String("indent"))) {
                            const auto indent = alignAttrs.value(QLatin1String("indent")).toString().toInt();
                            format.setIndent(indent);
                        }

                        if (alignAttrs.hasAttribute(QLatin1String("textRotation"))) {
                            const auto rotation = alignAttrs.value(QLatin1String("textRotation")).toString().toInt();
                            format.setRotation(rotation);
                        }

                        if (alignAttrs.hasAttribute(QLatin1String("wrapText")))
                            format.setTextWrap(true);

                        if (alignAttrs.hasAttribute(QLatin1String("shrinkToFit")))
                            format.setShrinkToFit(true);

                    }
                }

                addXfFormat(format, true);
            }
        }
    }

    if (reader.hasError())
        qWarning()<<reader.errorString();

    if (hasCount && (count != m_xf_formatsList.size()))
        qWarning("error read CellXfs");

    return true;
}

bool Styles::readDxfs(QXmlStreamReader &reader)
{
    Q_ASSERT(reader.name() == QLatin1String("dxfs"));
    const auto& attributes = reader.attributes();
    const auto hasCount = attributes.hasAttribute(QLatin1String("count"));
    const auto count = hasCount ? attributes.value(QLatin1String("count")).toString().toInt() : -1;
    while (!reader.atEnd() && !(reader.tokenType() == QXmlStreamReader::EndElement
                                && reader.name() == QLatin1String("dxfs"))) {
        reader.readNextStartElement();
        if (reader.tokenType() == QXmlStreamReader::StartElement) {
            if (reader.name() == QLatin1String("dxf"))
                readDxf(reader);
        }
    }
    if (reader.hasError())
        qWarning()<<reader.errorString();

    if (hasCount && (count != m_dxf_formatsList.size()))
        qWarning("error read dxfs");

    return true;
}

bool Styles::readDxf(QXmlStreamReader &reader)
{
    Q_ASSERT(reader.name() == QLatin1String("dxf"));
    Format format;
    while (!reader.atEnd() && !(reader.name() == QLatin1String("dxf") && reader.tokenType() == QXmlStreamReader::EndElement)) {
        reader.readNextStartElement();
        if (reader.tokenType() == QXmlStreamReader::StartElement) {
            if (reader.name() == QLatin1String("numFmt")) {
                const auto& attributes = reader.attributes();
                const auto id = attributes.value(QLatin1String("numFmtId")).toString().toInt();
                QString code = attributes.value(QLatin1String("formatCode")).toString();
                format.setNumberFormat(id, code);
            } else if (reader.name() == QLatin1String("font")) {
                readFont(reader, format);
            } else if (reader.name() == QLatin1String("fill")) {
                readFill(reader, format);
            } else if (reader.name() == QLatin1String("border")) {
                readBorder(reader, format);
            }
        }
    }
    addDxfFormat(format, true);
    return true;
}

bool Styles::readColors(QXmlStreamReader &reader)
{
    Q_ASSERT(reader.name() == QLatin1String("colors"));
    while (!reader.atEnd() && !(reader.name() == QLatin1String("colors") && reader.tokenType() == QXmlStreamReader::EndElement)) {
        reader.readNextStartElement();
        if (reader.tokenType() == QXmlStreamReader::StartElement) {
            if (reader.name() == QLatin1String("indexedColors")) {
                readIndexedColors(reader);
            } else if (reader.name() == QLatin1String("mruColors")) {

            }
        }
    }
    return true;
}

bool Styles::readIndexedColors(QXmlStreamReader &reader)
{
    Q_ASSERT(reader.name() == QLatin1String("indexedColors"));
    m_indexedColors.clear();
    while (!reader.atEnd() && !(reader.name() == QLatin1String("indexedColors") && reader.tokenType() == QXmlStreamReader::EndElement)) {
        reader.readNextStartElement();
        if (reader.tokenType() == QXmlStreamReader::StartElement) {
            if (reader.name() == QLatin1String("rgbColor")) {
                const auto& color = reader.attributes().value(QLatin1String("rgb")).toString();
                m_indexedColors.append(XlsxColor::fromARGBString(color));
            }
        }
    }
    if (!m_indexedColors.isEmpty())
        m_isIndexedColorsDefault = false;
    return true;
}

bool Styles::loadFromXmlFile(QIODevice *device)
{
    QXmlStreamReader reader(device);
    while (!reader.atEnd()) {
        QXmlStreamReader::TokenType token = reader.readNext();
        if (token == QXmlStreamReader::StartElement) {
            if (reader.name() == QLatin1String("numFmts")) {
                readNumFmts(reader);
            } else if (reader.name() == QLatin1String("fonts")) {
                readFonts(reader);
            } else if (reader.name() == QLatin1String("fills")) {
                readFills(reader);
            } else if (reader.name() == QLatin1String("borders")) {
                readBorders(reader);
            } else if (reader.name() == QLatin1String("cellStyleXfs")) {

                readCellStyleXfs(reader);

            } else if (reader.name() == QLatin1String("cellXfs")) {
                readCellXfs(reader);
            } else if (reader.name() == QLatin1String("cellStyles")) {

                // cellStyles

            } else if (reader.name() == QLatin1String("dxfs")) {
                readDxfs(reader);
            } else if (reader.name() == QLatin1String("colors")) {
                readColors(reader);
            }
        }

        if (reader.hasError()) {
            qDebug()<<"Error when read style file: "<<reader.errorString();
        }
    }
    return true;
}

#if QT_VERSION >= 0x050600
QColor Styles::getColorByIndex(int idx)
{
    // #if QT_VERSION >= 0x050600

    if (m_indexedColors.isEmpty()) {
        m_indexedColors = {
            QColor(QRgba64::fromArgb32(0x000000)), QColor(QRgba64::fromArgb32(0xFFFFFF)), QColor(QRgba64::fromArgb32(0xFF0000)), QColor(QRgba64::fromArgb32(0x00FF00)),
            QColor(QRgba64::fromArgb32(0x0000FF)), QColor(QRgba64::fromArgb32(0xFFFF00)), QColor(QRgba64::fromArgb32(0xFF00FF)), QColor(QRgba64::fromArgb32(0x00FFFF)),
            QColor(QRgba64::fromArgb32(0x000000)), QColor(QRgba64::fromArgb32(0xFFFFFF)), QColor(QRgba64::fromArgb32(0xFF0000)), QColor(QRgba64::fromArgb32(0x00FF00)),
            QColor(QRgba64::fromArgb32(0x0000FF)), QColor(QRgba64::fromArgb32(0xFFFF00)), QColor(QRgba64::fromArgb32(0xFF00FF)), QColor(QRgba64::fromArgb32(0x00FFFF)),
            QColor(QRgba64::fromArgb32(0x800000)), QColor(QRgba64::fromArgb32(0x008000)), QColor(QRgba64::fromArgb32(0x000080)), QColor(QRgba64::fromArgb32(0x808000)),
            QColor(QRgba64::fromArgb32(0x800080)), QColor(QRgba64::fromArgb32(0x008080)), QColor(QRgba64::fromArgb32(0xC0C0C0)), QColor(QRgba64::fromArgb32(0x808080)),
            QColor(QRgba64::fromArgb32(0x9999FF)), QColor(QRgba64::fromArgb32(0x993366)), QColor(QRgba64::fromArgb32(0xFFFFCC)), QColor(QRgba64::fromArgb32(0xCCFFFF)),
            QColor(QRgba64::fromArgb32(0x660066)), QColor(QRgba64::fromArgb32(0xFF8080)), QColor(QRgba64::fromArgb32(0x0066CC)), QColor(QRgba64::fromArgb32(0xCCCCFF)),
            QColor(QRgba64::fromArgb32(0x000080)), QColor(QRgba64::fromArgb32(0xFF00FF)), QColor(QRgba64::fromArgb32(0xFFFF00)), QColor(QRgba64::fromArgb32(0x00FFFF)),
            QColor(QRgba64::fromArgb32(0x800080)), QColor(QRgba64::fromArgb32(0x800000)), QColor(QRgba64::fromArgb32(0x008080)), QColor(QRgba64::fromArgb32(0x0000FF)),
            QColor(QRgba64::fromArgb32(0x00CCFF)), QColor(QRgba64::fromArgb32(0xCCFFFF)), QColor(QRgba64::fromArgb32(0xCCFFCC)), QColor(QRgba64::fromArgb32(0xFFFF99)),
            QColor(QRgba64::fromArgb32(0x99CCFF)), QColor(QRgba64::fromArgb32(0xFF99CC)), QColor(QRgba64::fromArgb32(0xCC99FF)), QColor(QRgba64::fromArgb32(0xFFCC99)),
            QColor(QRgba64::fromArgb32(0x3366FF)), QColor(QRgba64::fromArgb32(0x33CCCC)), QColor(QRgba64::fromArgb32(0x99CC00)), QColor(QRgba64::fromArgb32(0xFFCC00)),
            QColor(QRgba64::fromArgb32(0xFF9900)), QColor(QRgba64::fromArgb32(0xFF6600)), QColor(QRgba64::fromArgb32(0x666699)), QColor(QRgba64::fromArgb32(0x969696)),
            QColor(QRgba64::fromArgb32(0x003366)), QColor(QRgba64::fromArgb32(0x339966)), QColor(QRgba64::fromArgb32(0x003300)), QColor(QRgba64::fromArgb32(0x333300)),
            QColor(QRgba64::fromArgb32(0x993300)), QColor(QRgba64::fromArgb32(0x993366)), QColor(QRgba64::fromArgb32(0x333399)), QColor(QRgba64::fromArgb32(0x333333)),
        };
        m_isIndexedColorsDefault = true;
    }
    if (idx < 0 || idx >= m_indexedColors.size())
        return QColor();
    return m_indexedColors[idx];
}
#endif

QT_END_NAMESPACE_XLSX
