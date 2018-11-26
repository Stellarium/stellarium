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

#include "xlsxconditionalformatting.h"
#include "xlsxconditionalformatting_p.h"
#include "xlsxworksheet.h"
#include "xlsxcellrange.h"
#include "xlsxstyles_p.h"

#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QDebug>

QT_BEGIN_NAMESPACE_XLSX

ConditionalFormattingPrivate::ConditionalFormattingPrivate()
{

}

ConditionalFormattingPrivate::ConditionalFormattingPrivate(const ConditionalFormattingPrivate &other)
    :QSharedData(other)
{

}

ConditionalFormattingPrivate::~ConditionalFormattingPrivate()
{

}

void ConditionalFormattingPrivate::writeCfVo(QXmlStreamWriter &writer, const XlsxCfVoData &cfvo) const
{
    writer.writeEmptyElement(QStringLiteral("cfvo"));
    QString type;
    switch(cfvo.type) {
    case ConditionalFormatting::VOT_Formula: type=QStringLiteral("formula"); break;
    case ConditionalFormatting::VOT_Max: type=QStringLiteral("max"); break;
    case ConditionalFormatting::VOT_Min: type=QStringLiteral("min"); break;
    case ConditionalFormatting::VOT_Num: type=QStringLiteral("num"); break;
    case ConditionalFormatting::VOT_Percent: type=QStringLiteral("percent"); break;
    case ConditionalFormatting::VOT_Percentile: type=QStringLiteral("percentile"); break;
    default: break;
    }
    writer.writeAttribute(QStringLiteral("type"), type);
    writer.writeAttribute(QStringLiteral("val"), cfvo.value);
    if (!cfvo.gte)
        writer.writeAttribute(QStringLiteral("gte"), QStringLiteral("0"));
}

/*!
 * \class ConditionalFormatting
 * \brief Conditional formatting for single cell or ranges
 * \inmodule QtXlsx
 *
 * The conditional formatting can be applied to a single cell or ranges of cells.
 */


/*!
    \enum ConditionalFormatting::HighlightRuleType

    \value Highlight_LessThan
    \value Highlight_LessThanOrEqual
    \value Highlight_Equal
    \value Highlight_NotEqual
    \value Highlight_GreaterThanOrEqual
    \value Highlight_GreaterThan
    \value Highlight_Between
    \value Highlight_NotBetween

    \value Highlight_ContainsText
    \value Highlight_NotContainsText
    \value Highlight_BeginsWith
    \value Highlight_EndsWith

    \value Highlight_TimePeriod

    \value Highlight_Duplicate
    \value Highlight_Unique

    \value Highlight_Blanks
    \value Highlight_NoBlanks
    \value Highlight_Errors
    \value Highlight_NoErrors

    \value Highlight_Top
    \value Highlight_TopPercent
    \value Highlight_Bottom
    \value Highlight_BottomPercent

    \value Highlight_AboveAverage
    \value Highlight_AboveOrEqualAverage
    \value Highlight_BelowAverage
    \value Highlight_BelowOrEqualAverage
    \value Highlight_AboveStdDev1
    \value Highlight_AboveStdDev2
    \value Highlight_AboveStdDev3
    \value Highlight_BelowStdDev1
    \value Highlight_BelowStdDev2
    \value Highlight_BelowStdDev3

    \value Highlight_Expression
*/

/*!
    \enum ConditionalFormatting::ValueObjectType

    \value VOT_Formula
    \value VOT_Max
    \value VOT_Min
    \value VOT_Num
    \value VOT_Percent
    \value VOT_Percentile
*/

/*!
    Construct a conditional formatting object
*/
ConditionalFormatting::ConditionalFormatting()
    :d(new ConditionalFormattingPrivate())
{

}

/*!
    Constructs a copy of \a other.
*/
ConditionalFormatting::ConditionalFormatting(const ConditionalFormatting &other)
    :d(other.d)
{

}

/*!
    Assigns \a other to this conditional formatting and returns a reference to
    this conditional formatting.
 */
ConditionalFormatting &ConditionalFormatting::operator=(const ConditionalFormatting &other)
{
    this->d = other.d;
    return *this;
}


/*!
 * Destroy the object.
 */
ConditionalFormatting::~ConditionalFormatting()
{
}

/*!
 * Add a hightlight rule with the given \a type, \a formula1, \a formula2,
 * \a format and \a stopIfTrue.
 * Return false if failed.
 */
bool ConditionalFormatting::addHighlightCellsRule(HighlightRuleType type, const QString &formula1, const QString &formula2, const Format &format, bool stopIfTrue)
{
    if (format.isEmpty())
        return false;

    bool skipFormula = false;

    QSharedPointer<XlsxCfRuleData> cfRule(new XlsxCfRuleData);
    if (type >= Highlight_LessThan && type <= Highlight_NotBetween) {
        cfRule->attrs[XlsxCfRuleData::A_type] = QStringLiteral("cellIs");
        QString op;
        switch (type) {
        case Highlight_Between: op = QStringLiteral("between"); break;
        case Highlight_Equal: op = QStringLiteral("equal"); break;
        case Highlight_GreaterThan: op = QStringLiteral("greaterThan"); break;
        case Highlight_GreaterThanOrEqual: op = QStringLiteral("greaterThanOrEqual"); break;
        case Highlight_LessThan: op = QStringLiteral("lessThan"); break;
        case Highlight_LessThanOrEqual: op = QStringLiteral("lessThanOrEqual"); break;
        case Highlight_NotBetween: op = QStringLiteral("notBetween"); break;
        case Highlight_NotEqual: op = QStringLiteral("notEqual"); break;
        default: break;
        }
        cfRule->attrs[XlsxCfRuleData::A_operator] = op;
    } else if (type >= Highlight_ContainsText && type <= Highlight_EndsWith) {
        if (type == Highlight_ContainsText) {
            cfRule->attrs[XlsxCfRuleData::A_type] = QStringLiteral("containsText");
            cfRule->attrs[XlsxCfRuleData::A_operator] = QStringLiteral("containsText");
            cfRule->attrs[XlsxCfRuleData::A_formula1_temp] = QStringLiteral("NOT(ISERROR(SEARCH(\"%1\",%2)))").arg(formula1);
        } else if (type == Highlight_NotContainsText) {
            cfRule->attrs[XlsxCfRuleData::A_type] = QStringLiteral("notContainsText");
            cfRule->attrs[XlsxCfRuleData::A_operator] = QStringLiteral("notContains");
            cfRule->attrs[XlsxCfRuleData::A_formula1_temp] = QStringLiteral("ISERROR(SEARCH(\"%2\",%1))").arg(formula1);
        } else if (type == Highlight_BeginsWith) {
            cfRule->attrs[XlsxCfRuleData::A_type] = QStringLiteral("beginsWith");
            cfRule->attrs[XlsxCfRuleData::A_operator] = QStringLiteral("beginsWith");
            cfRule->attrs[XlsxCfRuleData::A_formula1_temp] = QStringLiteral("LEFT(%2,LEN(\"%1\"))=\"%1\"").arg(formula1);
        } else {
            cfRule->attrs[XlsxCfRuleData::A_type] = QStringLiteral("endsWith");
            cfRule->attrs[XlsxCfRuleData::A_operator] = QStringLiteral("endsWith");
            cfRule->attrs[XlsxCfRuleData::A_formula1_temp] = QStringLiteral("RIGHT(%2,LEN(\"%1\"))=\"%1\"").arg(formula1);
        }
        cfRule->attrs[XlsxCfRuleData::A_text] = formula1;
        skipFormula = true;
    } else if (type == Highlight_TimePeriod) {
        cfRule->attrs[XlsxCfRuleData::A_type] = QStringLiteral("timePeriod");
        //:Todo
        return false;
    } else if (type == Highlight_Duplicate) {
        cfRule->attrs[XlsxCfRuleData::A_type] = QStringLiteral("duplicateValues");
    } else if (type == Highlight_Unique) {
        cfRule->attrs[XlsxCfRuleData::A_type] = QStringLiteral("uniqueValues");
    } else if (type == Highlight_Errors) {
        cfRule->attrs[XlsxCfRuleData::A_type] = QStringLiteral("containsErrors");
        cfRule->attrs[XlsxCfRuleData::A_formula1_temp] = QStringLiteral("ISERROR(%1)");
        skipFormula = true;
    } else if (type == Highlight_NoErrors) {
        cfRule->attrs[XlsxCfRuleData::A_type] = QStringLiteral("notContainsErrors");
        cfRule->attrs[XlsxCfRuleData::A_formula1_temp] = QStringLiteral("NOT(ISERROR(%1))");
        skipFormula = true;
    } else if (type == Highlight_Blanks) {
        cfRule->attrs[XlsxCfRuleData::A_type] = QStringLiteral("containsBlanks");
        cfRule->attrs[XlsxCfRuleData::A_formula1_temp] = QStringLiteral("LEN(TRIM(%1))=0");
        skipFormula = true;
    } else if (type == Highlight_NoBlanks) {
        cfRule->attrs[XlsxCfRuleData::A_type] = QStringLiteral("notContainsBlanks");
        cfRule->attrs[XlsxCfRuleData::A_formula1_temp] = QStringLiteral("LEN(TRIM(%1))>0");
        skipFormula = true;
    } else if (type >= Highlight_Top && type <= Highlight_BottomPercent) {
        cfRule->attrs[XlsxCfRuleData::A_type] = QStringLiteral("top10");
        if (type == Highlight_Bottom || type == Highlight_BottomPercent)
            cfRule->attrs[XlsxCfRuleData::A_bottom] = QStringLiteral("1");
        if (type == Highlight_TopPercent || type == Highlight_BottomPercent)
            cfRule->attrs[XlsxCfRuleData::A_percent] = QStringLiteral("1");
        cfRule->attrs[XlsxCfRuleData::A_rank] = !formula1.isEmpty() ? formula1 : QStringLiteral("10");
        skipFormula = true;
    } else if (type >= Highlight_AboveAverage && type <= Highlight_BelowStdDev3) {
        cfRule->attrs[XlsxCfRuleData::A_type] = QStringLiteral("aboveAverage");
        if (type >= Highlight_BelowAverage && type <= Highlight_BelowStdDev3)
            cfRule->attrs[XlsxCfRuleData::A_aboveAverage] = QStringLiteral("0");
        if (type == Highlight_AboveOrEqualAverage || type == Highlight_BelowOrEqualAverage)
            cfRule->attrs[XlsxCfRuleData::A_equalAverage] = QStringLiteral("1");
        if (type == Highlight_AboveStdDev1 || type == Highlight_BelowStdDev1)
            cfRule->attrs[XlsxCfRuleData::A_stdDev] = QStringLiteral("1");
        else if (type == Highlight_AboveStdDev2 || type == Highlight_BelowStdDev2)
            cfRule->attrs[XlsxCfRuleData::A_stdDev] = QStringLiteral("2");
        else if (type == Highlight_AboveStdDev3 || type == Highlight_BelowStdDev3)
            cfRule->attrs[XlsxCfRuleData::A_stdDev] = QStringLiteral("3");
    } else if (type == Highlight_Expression){
        cfRule->attrs[XlsxCfRuleData::A_type] = QStringLiteral("expression");
    } else {
        return false;
    }

    cfRule->dxfFormat = format;
    if (stopIfTrue)
        cfRule->attrs[XlsxCfRuleData::A_stopIfTrue] = true;
    if (!skipFormula) {
        if (!formula1.isEmpty())
            cfRule->attrs[XlsxCfRuleData::A_formula1] = formula1.startsWith(QLatin1String("=")) ? formula1.mid(1) : formula1;
        if (!formula2.isEmpty())
            cfRule->attrs[XlsxCfRuleData::A_formula2] = formula2.startsWith(QLatin1String("=")) ? formula2.mid(1) : formula2;
    }
    d->cfRules.append(cfRule);
    return true;
}

/*!
 * \overload
 *
 * Add a hightlight rule with the given \a type \a format and \a stopIfTrue.
 */
bool ConditionalFormatting::addHighlightCellsRule(HighlightRuleType type, const Format &format, bool stopIfTrue)
{
    if ((type >= Highlight_AboveAverage && type <= Highlight_BelowStdDev3)
            || (type >= Highlight_Duplicate && type <= Highlight_NoErrors)) {
        return addHighlightCellsRule(type, QString(), QString(), format, stopIfTrue);
    }

    return false;
}

/*!
 * \overload
 *
 * Add a hightlight rule with the given \a type, \a formula, \a format and \a stopIfTrue.
 * Return false if failed.
 */
bool ConditionalFormatting::addHighlightCellsRule(HighlightRuleType type, const QString &formula, const Format &format, bool stopIfTrue)
{
    if (type == Highlight_Between || type == Highlight_NotBetween)
        return false;

    return addHighlightCellsRule(type, formula, QString(), format, stopIfTrue);
}

/*!
 * Add a dataBar rule with the given \a color, \a type1, \a val1
 * , \a type2, \a val2, \a showData and \a stopIfTrue.
 * Return false if failed.
 */
bool ConditionalFormatting::addDataBarRule(const QColor &color, ValueObjectType type1, const QString &val1, ValueObjectType type2, const QString &val2, bool showData, bool stopIfTrue)
{
    QSharedPointer<XlsxCfRuleData> cfRule(new XlsxCfRuleData);

    cfRule->attrs[XlsxCfRuleData::A_type] = QStringLiteral("dataBar");
    cfRule->attrs[XlsxCfRuleData::A_color1] = XlsxColor(color);
    if (stopIfTrue)
        cfRule->attrs[XlsxCfRuleData::A_stopIfTrue] = true;
    if (!showData)
        cfRule->attrs[XlsxCfRuleData::A_hideData] = true;

    XlsxCfVoData cfvo1(type1, val1);
    XlsxCfVoData cfvo2(type2, val2);
    cfRule->attrs[XlsxCfRuleData::A_cfvo1] = QVariant::fromValue(cfvo1);
    cfRule->attrs[XlsxCfRuleData::A_cfvo2] = QVariant::fromValue(cfvo2);

    d->cfRules.append(cfRule);
    return true;
}

/*!
 * \overload
 * Add a dataBar rule with the given \a color, \a showData and \a stopIfTrue.
 */
bool ConditionalFormatting::addDataBarRule(const QColor &color, bool showData, bool stopIfTrue)
{
    return addDataBarRule(color, VOT_Min, QStringLiteral("0"), VOT_Max, QStringLiteral("0"), showData, stopIfTrue);
}

/*!
 * Add a colorScale rule with the given \a minColor, \a maxColor and \a stopIfTrue.
 * Return false if failed.
 */
bool ConditionalFormatting::add2ColorScaleRule(const QColor &minColor, const QColor &maxColor, bool stopIfTrue)
{
    ValueObjectType type1 = VOT_Min;
    ValueObjectType type2 = VOT_Max;
    QString val1 = QStringLiteral("0");
    QString val2 = QStringLiteral("0");

    QSharedPointer<XlsxCfRuleData> cfRule(new XlsxCfRuleData);

    cfRule->attrs[XlsxCfRuleData::A_type] = QStringLiteral("colorScale");
    cfRule->attrs[XlsxCfRuleData::A_color1] = XlsxColor(minColor);
    cfRule->attrs[XlsxCfRuleData::A_color2] = XlsxColor(maxColor);
    if (stopIfTrue)
        cfRule->attrs[XlsxCfRuleData::A_stopIfTrue] = true;

    XlsxCfVoData cfvo1(type1, val1);
    XlsxCfVoData cfvo2(type2, val2);
    cfRule->attrs[XlsxCfRuleData::A_cfvo1] = QVariant::fromValue(cfvo1);
    cfRule->attrs[XlsxCfRuleData::A_cfvo2] = QVariant::fromValue(cfvo2);

    d->cfRules.append(cfRule);
    return true;
}

/*!
 * Add a colorScale rule with the given \a minColor, \a midColor, \a maxColor and \a stopIfTrue.
 * Return false if failed.
 */
bool ConditionalFormatting::add3ColorScaleRule(const QColor &minColor, const QColor &midColor, const QColor &maxColor, bool stopIfTrue)
{
    ValueObjectType type1 = VOT_Min;
    ValueObjectType type2 = VOT_Percent;
    ValueObjectType type3 = VOT_Max;
    QString val1 = QStringLiteral("0");
    QString val2 = QStringLiteral("50");
    QString val3 = QStringLiteral("0");

    QSharedPointer<XlsxCfRuleData> cfRule(new XlsxCfRuleData);

    cfRule->attrs[XlsxCfRuleData::A_type] = QStringLiteral("colorScale");
    cfRule->attrs[XlsxCfRuleData::A_color1] = XlsxColor(minColor);
    cfRule->attrs[XlsxCfRuleData::A_color2] = XlsxColor(midColor);
    cfRule->attrs[XlsxCfRuleData::A_color3] = XlsxColor(maxColor);

    if (stopIfTrue)
        cfRule->attrs[XlsxCfRuleData::A_stopIfTrue] = true;

    XlsxCfVoData cfvo1(type1, val1);
    XlsxCfVoData cfvo2(type2, val2);
    XlsxCfVoData cfvo3(type3, val3);
    cfRule->attrs[XlsxCfRuleData::A_cfvo1] = QVariant::fromValue(cfvo1);
    cfRule->attrs[XlsxCfRuleData::A_cfvo2] = QVariant::fromValue(cfvo2);
    cfRule->attrs[XlsxCfRuleData::A_cfvo3] = QVariant::fromValue(cfvo3);

    d->cfRules.append(cfRule);
    return true;
}

/*!
    Returns the ranges on which the validation will be applied.
 */
QList<CellRange> ConditionalFormatting::ranges() const
{
    return d->ranges;
}

/*!
    Add the \a cell on which the conditional formatting will apply to.
 */
void ConditionalFormatting::addCell(const CellReference &cell)
{
    d->ranges.append(CellRange(cell, cell));
}

/*!
    \overload
    Add the cell(\a row, \a col) on which the conditional formatting will apply to.
 */
void ConditionalFormatting::addCell(int row, int col)
{
    d->ranges.append(CellRange(row, col, row, col));
}

/*!
    \overload
    Add the range(\a firstRow, \a firstCol, \a lastRow, \a lastCol) on
    which the conditional formatting will apply to.
 */
void ConditionalFormatting::addRange(int firstRow, int firstCol, int lastRow, int lastCol)
{
    d->ranges.append(CellRange(firstRow, firstCol, lastRow, lastCol));
}

/*!
    Add the \a range on which the conditional formatting will apply to.
 */
void ConditionalFormatting::addRange(const CellRange &range)
{
    d->ranges.append(range);
}

bool ConditionalFormattingPrivate::readCfRule(QXmlStreamReader &reader, XlsxCfRuleData *rule, Styles *styles)
{
    Q_ASSERT(reader.name() == QLatin1String("cfRule"));
    QXmlStreamAttributes attrs = reader.attributes();
    if (attrs.hasAttribute(QLatin1String("type")))
        rule->attrs[XlsxCfRuleData::A_type] = attrs.value(QLatin1String("type")).toString();
    if (attrs.hasAttribute(QLatin1String("dxfId"))) {
        int id = attrs.value(QLatin1String("dxfId")).toString().toInt();
        if (styles)
            rule->dxfFormat = styles->dxfFormat(id);
        else
            rule->dxfFormat.setDxfIndex(id);
    }
    rule->priority = attrs.value(QLatin1String("priority")).toString().toInt();
    if (attrs.value(QLatin1String("stopIfTrue")) == QLatin1String("1")) {
        //default is false
        rule->attrs[XlsxCfRuleData::A_stopIfTrue] = QLatin1String("1");
    }
    if (attrs.value(QLatin1String("aboveAverage")) == QLatin1String("0")) {
        //default is true
        rule->attrs[XlsxCfRuleData::A_aboveAverage] = QLatin1String("0");
    }
    if (attrs.value(QLatin1String("percent")) == QLatin1String("1")) {
        //default is false
        rule->attrs[XlsxCfRuleData::A_percent] = QLatin1String("1");
    }
    if (attrs.value(QLatin1String("bottom")) == QLatin1String("1")) {
        //default is false
        rule->attrs[XlsxCfRuleData::A_bottom] = QLatin1String("1");
    }
    if (attrs.hasAttribute(QLatin1String("operator")))
        rule->attrs[XlsxCfRuleData::A_operator] = attrs.value(QLatin1String("operator")).toString();

    if (attrs.hasAttribute(QLatin1String("text")))
        rule->attrs[XlsxCfRuleData::A_text] = attrs.value(QLatin1String("text")).toString();

    if (attrs.hasAttribute(QLatin1String("timePeriod")))
        rule->attrs[XlsxCfRuleData::A_timePeriod] = attrs.value(QLatin1String("timePeriod")).toString();

    if (attrs.hasAttribute(QLatin1String("rank")))
        rule->attrs[XlsxCfRuleData::A_rank] = attrs.value(QLatin1String("rank")).toString();

    if (attrs.hasAttribute(QLatin1String("stdDev")))
        rule->attrs[XlsxCfRuleData::A_stdDev] = attrs.value(QLatin1String("stdDev")).toString();

    if (attrs.value(QLatin1String("equalAverage")) == QLatin1String("1")) {
        //default is false
        rule->attrs[XlsxCfRuleData::A_equalAverage] = QLatin1String("1");
    }

    while (!reader.atEnd()) {
        reader.readNextStartElement();
        if (reader.tokenType() == QXmlStreamReader::StartElement) {
            if (reader.name() == QLatin1String("formula")) {
                QString f = reader.readElementText();
                if (!rule->attrs.contains(XlsxCfRuleData::A_formula1))
                    rule->attrs[XlsxCfRuleData::A_formula1] = f;
                else if (!rule->attrs.contains(XlsxCfRuleData::A_formula2))
                    rule->attrs[XlsxCfRuleData::A_formula2] = f;
                else if (!rule->attrs.contains(XlsxCfRuleData::A_formula3))
                    rule->attrs[XlsxCfRuleData::A_formula3] = f;
            } else if (reader.name() == QLatin1String("dataBar")) {
                readCfDataBar(reader, rule);
            } else if (reader.name() == QLatin1String("colorScale")) {
                readCfColorScale(reader, rule);
            }
        }
        if (reader.tokenType() == QXmlStreamReader::EndElement
                && reader.name() == QStringLiteral("conditionalFormatting")) {
            break;
        }
    }
    return true;
}

bool ConditionalFormattingPrivate::readCfDataBar(QXmlStreamReader &reader, XlsxCfRuleData *rule)
{
    Q_ASSERT(reader.name() == QLatin1String("dataBar"));
    QXmlStreamAttributes attrs = reader.attributes();
    if (attrs.value(QLatin1String("showValue")) == QLatin1String("0"))
        rule->attrs[XlsxCfRuleData::A_hideData] = QStringLiteral("1");

    while (!reader.atEnd()) {
        reader.readNextStartElement();
        if (reader.tokenType() == QXmlStreamReader::StartElement) {
            if (reader.name() == QLatin1String("cfvo")) {
                XlsxCfVoData data;
                readCfVo(reader, data);
                if (!rule->attrs.contains(XlsxCfRuleData::A_cfvo1))
                    rule->attrs[XlsxCfRuleData::A_cfvo1] = QVariant::fromValue(data);
                else
                    rule->attrs[XlsxCfRuleData::A_cfvo2] = QVariant::fromValue(data);
            } else if (reader.name() == QLatin1String("color")) {
                XlsxColor color;
                color.loadFromXml(reader);
                rule->attrs[XlsxCfRuleData::A_color1] = color;
            }
        }
        if (reader.tokenType() == QXmlStreamReader::EndElement
                && reader.name() == QStringLiteral("dataBar")) {
            break;
        }
    }

    return true;
}

bool ConditionalFormattingPrivate::readCfColorScale(QXmlStreamReader &reader, XlsxCfRuleData *rule)
{
    Q_ASSERT(reader.name() == QLatin1String("colorScale"));

    while (!reader.atEnd()) {
        reader.readNextStartElement();
        if (reader.tokenType() == QXmlStreamReader::StartElement) {
            if (reader.name() == QLatin1String("cfvo")) {
                XlsxCfVoData data;
                readCfVo(reader, data);
                if (!rule->attrs.contains(XlsxCfRuleData::A_cfvo1))
                    rule->attrs[XlsxCfRuleData::A_cfvo1] = QVariant::fromValue(data);
                else if (!rule->attrs.contains(XlsxCfRuleData::A_cfvo2))
                    rule->attrs[XlsxCfRuleData::A_cfvo2] = QVariant::fromValue(data);
                else
                    rule->attrs[XlsxCfRuleData::A_cfvo2] = QVariant::fromValue(data);
            } else if (reader.name() == QLatin1String("color")) {
                XlsxColor color;
                color.loadFromXml(reader);
                if (!rule->attrs.contains(XlsxCfRuleData::A_color1))
                    rule->attrs[XlsxCfRuleData::A_color1] = color;
                else if (!rule->attrs.contains(XlsxCfRuleData::A_color2))
                    rule->attrs[XlsxCfRuleData::A_color2] = color;
                else
                    rule->attrs[XlsxCfRuleData::A_color3] = color;
            }
        }
        if (reader.tokenType() == QXmlStreamReader::EndElement
                && reader.name() == QStringLiteral("colorScale")) {
            break;
        }
    }

    return true;
}

bool ConditionalFormattingPrivate::readCfVo(QXmlStreamReader &reader, XlsxCfVoData &cfvo)
{
    Q_ASSERT(reader.name() == QStringLiteral("cfvo"));

    QXmlStreamAttributes attrs = reader.attributes();

    QString type = attrs.value(QLatin1String("type")).toString();
    ConditionalFormatting::ValueObjectType t;
    if (type == QLatin1String("formula"))
        t = ConditionalFormatting::VOT_Formula;
    else if (type == QLatin1String("max"))
        t = ConditionalFormatting::VOT_Max;
    else if (type == QLatin1String("min"))
        t = ConditionalFormatting::VOT_Min;
    else if (type == QLatin1String("num"))
        t = ConditionalFormatting::VOT_Num;
    else if (type == QLatin1String("percent"))
        t = ConditionalFormatting::VOT_Percent;
    else //if (type == QLatin1String("percentile"))
        t = ConditionalFormatting::VOT_Percentile;

    cfvo.type = t;
    cfvo.value = attrs.value(QLatin1String("val")).toString();
    if (attrs.value(QLatin1String("gte")) == QLatin1String("0")) {
        //default is true
        cfvo.gte = false;
    }
    return true;
}

bool ConditionalFormatting::loadFromXml(QXmlStreamReader &reader, Styles *styles)
{
    Q_ASSERT(reader.name() == QStringLiteral("conditionalFormatting"));

    d->ranges.clear();
    d->cfRules.clear();
    QXmlStreamAttributes attrs = reader.attributes();
    QString sqref = attrs.value(QLatin1String("sqref")).toString();
    foreach (QString range, sqref.split(QLatin1Char(' ')))
        this->addRange(range);

    while (!reader.atEnd()) {
        reader.readNextStartElement();
        if (reader.tokenType() == QXmlStreamReader::StartElement) {
            if (reader.name() == QLatin1String("cfRule")) {
                QSharedPointer<XlsxCfRuleData> cfRule(new XlsxCfRuleData);
                d->readCfRule(reader, cfRule.data(), styles);
                d->cfRules.append(cfRule);
            }
        }
        if (reader.tokenType() == QXmlStreamReader::EndElement
                && reader.name() == QStringLiteral("conditionalFormatting")) {
            break;
        }
    }


    return true;
}

bool ConditionalFormatting::saveToXml(QXmlStreamWriter &writer) const
{
    writer.writeStartElement(QStringLiteral("conditionalFormatting"));
    QStringList sqref;
    foreach (CellRange range, ranges())
        sqref.append(range.toString());
    writer.writeAttribute(QStringLiteral("sqref"), sqref.join(QLatin1Char(' ')));

    for (int i=0; i<d->cfRules.size(); ++i) {
        const QSharedPointer<XlsxCfRuleData> &rule = d->cfRules[i];
        writer.writeStartElement(QStringLiteral("cfRule"));
        writer.writeAttribute(QStringLiteral("type"), rule->attrs[XlsxCfRuleData::A_type].toString());
        if (rule->dxfFormat.dxfIndexValid())
            writer.writeAttribute(QStringLiteral("dxfId"), QString::number(rule->dxfFormat.dxfIndex()));
        writer.writeAttribute(QStringLiteral("priority"), QString::number(rule->priority));
        if (rule->attrs.contains(XlsxCfRuleData::A_stopIfTrue))
            writer.writeAttribute(QStringLiteral("stopIfTrue"), rule->attrs[XlsxCfRuleData::A_stopIfTrue].toString());
        if (rule->attrs.contains(XlsxCfRuleData::A_aboveAverage))
            writer.writeAttribute(QStringLiteral("aboveAverage"), rule->attrs[XlsxCfRuleData::A_aboveAverage].toString());
        if (rule->attrs.contains(XlsxCfRuleData::A_percent))
            writer.writeAttribute(QStringLiteral("percent"), rule->attrs[XlsxCfRuleData::A_percent].toString());
        if (rule->attrs.contains(XlsxCfRuleData::A_bottom))
            writer.writeAttribute(QStringLiteral("bottom"), rule->attrs[XlsxCfRuleData::A_bottom].toString());
        if (rule->attrs.contains(XlsxCfRuleData::A_operator))
            writer.writeAttribute(QStringLiteral("operator"), rule->attrs[XlsxCfRuleData::A_operator].toString());
        if (rule->attrs.contains(XlsxCfRuleData::A_text))
            writer.writeAttribute(QStringLiteral("text"), rule->attrs[XlsxCfRuleData::A_text].toString());
        if (rule->attrs.contains(XlsxCfRuleData::A_timePeriod))
            writer.writeAttribute(QStringLiteral("timePeriod"), rule->attrs[XlsxCfRuleData::A_timePeriod].toString());
        if (rule->attrs.contains(XlsxCfRuleData::A_rank))
            writer.writeAttribute(QStringLiteral("rank"), rule->attrs[XlsxCfRuleData::A_rank].toString());
        if (rule->attrs.contains(XlsxCfRuleData::A_stdDev))
            writer.writeAttribute(QStringLiteral("stdDev"), rule->attrs[XlsxCfRuleData::A_stdDev].toString());
        if (rule->attrs.contains(XlsxCfRuleData::A_equalAverage))
            writer.writeAttribute(QStringLiteral("equalAverage"), rule->attrs[XlsxCfRuleData::A_equalAverage].toString());

        if (rule->attrs[XlsxCfRuleData::A_type] == QLatin1String("dataBar")) {
            writer.writeStartElement(QStringLiteral("dataBar"));
            if (rule->attrs.contains(XlsxCfRuleData::A_hideData))
                writer.writeAttribute(QStringLiteral("showValue"), QStringLiteral("0"));
            d->writeCfVo(writer, rule->attrs[XlsxCfRuleData::A_cfvo1].value<XlsxCfVoData>());
            d->writeCfVo(writer, rule->attrs[XlsxCfRuleData::A_cfvo2].value<XlsxCfVoData>());
            rule->attrs[XlsxCfRuleData::A_color1].value<XlsxColor>().saveToXml(writer);
            writer.writeEndElement();//dataBar
        } else if (rule->attrs[XlsxCfRuleData::A_type] == QLatin1String("colorScale")) {
            writer.writeStartElement(QStringLiteral("colorScale"));
            d->writeCfVo(writer, rule->attrs[XlsxCfRuleData::A_cfvo1].value<XlsxCfVoData>());
            d->writeCfVo(writer, rule->attrs[XlsxCfRuleData::A_cfvo2].value<XlsxCfVoData>());
            if (rule->attrs.contains(XlsxCfRuleData::A_cfvo3))
                d->writeCfVo(writer, rule->attrs[XlsxCfRuleData::A_cfvo3].value<XlsxCfVoData>());

            rule->attrs[XlsxCfRuleData::A_color1].value<XlsxColor>().saveToXml(writer);
            rule->attrs[XlsxCfRuleData::A_color2].value<XlsxColor>().saveToXml(writer);
            if (rule->attrs.contains(XlsxCfRuleData::A_color3))
                rule->attrs[XlsxCfRuleData::A_color3].value<XlsxColor>().saveToXml(writer);

            writer.writeEndElement();//colorScale
        }


        if (rule->attrs.contains(XlsxCfRuleData::A_formula1_temp)) {
            QString startCell = ranges()[0].toString().split(QLatin1Char(':'))[0];
            writer.writeTextElement(QStringLiteral("formula"), rule->attrs[XlsxCfRuleData::A_formula1_temp].toString().arg(startCell));
        } else if (rule->attrs.contains(XlsxCfRuleData::A_formula1)) {
            writer.writeTextElement(QStringLiteral("formula"), rule->attrs[XlsxCfRuleData::A_formula1].toString());
        }
        if (rule->attrs.contains(XlsxCfRuleData::A_formula2))
            writer.writeTextElement(QStringLiteral("formula"), rule->attrs[XlsxCfRuleData::A_formula2].toString());
        if (rule->attrs.contains(XlsxCfRuleData::A_formula3))
            writer.writeTextElement(QStringLiteral("formula"), rule->attrs[XlsxCfRuleData::A_formula3].toString());

        writer.writeEndElement(); //cfRule
    }

    writer.writeEndElement(); //conditionalFormatting
    return true;
}

QT_END_NAMESPACE_XLSX
