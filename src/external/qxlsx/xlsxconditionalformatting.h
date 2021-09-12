// xlsxconditionalformatting.h

#ifndef QXLSX_XLSXCONDITIONALFORMATTING_H
#define QXLSX_XLSXCONDITIONALFORMATTING_H

#include <QtGlobal>
#include <QString>
#include <QList>
#include <QColor>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QSharedDataPointer>

#include "xlsxglobal.h"
#include "xlsxcellrange.h"
#include "xlsxcellreference.h"

class ConditionalFormattingTest;

QT_BEGIN_NAMESPACE_XLSX

class Format;
class Worksheet;
class Styles;
class ConditionalFormattingPrivate;

class ConditionalFormatting
{
public:
    enum HighlightRuleType {
        Highlight_LessThan,
        Highlight_LessThanOrEqual,
        Highlight_Equal,
        Highlight_NotEqual,
        Highlight_GreaterThanOrEqual,
        Highlight_GreaterThan,
        Highlight_Between,
        Highlight_NotBetween,

        Highlight_ContainsText,
        Highlight_NotContainsText,
        Highlight_BeginsWith,
        Highlight_EndsWith,

        Highlight_TimePeriod,

        Highlight_Duplicate,
        Highlight_Unique,
        Highlight_Blanks,
        Highlight_NoBlanks,
        Highlight_Errors,
        Highlight_NoErrors,

        Highlight_Top,
        Highlight_TopPercent,
        Highlight_Bottom,
        Highlight_BottomPercent,

        Highlight_AboveAverage,
        Highlight_AboveOrEqualAverage,
        Highlight_AboveStdDev1,
        Highlight_AboveStdDev2,
        Highlight_AboveStdDev3,
        Highlight_BelowAverage,
        Highlight_BelowOrEqualAverage,
        Highlight_BelowStdDev1,
        Highlight_BelowStdDev2,
        Highlight_BelowStdDev3,

        Highlight_Expression
    };

    enum ValueObjectType
    {
        VOT_Formula,
        VOT_Max,
        VOT_Min,
        VOT_Num,
        VOT_Percent,
        VOT_Percentile
    };

public:
    ConditionalFormatting();
    ConditionalFormatting(const ConditionalFormatting &other);
    ~ConditionalFormatting();

public:
    bool addHighlightCellsRule(HighlightRuleType type, const Format &format, bool stopIfTrue=false);
    bool addHighlightCellsRule(HighlightRuleType type, const QString &formula1, const Format &format, bool stopIfTrue=false);
    bool addHighlightCellsRule(HighlightRuleType type, const QString &formula1, const QString &formula2, const Format &format, bool stopIfTrue=false);
    bool addDataBarRule(const QColor &color, bool showData=true, bool stopIfTrue=false);
    bool addDataBarRule(const QColor &color, ValueObjectType type1, const QString &val1, ValueObjectType type2, const QString &val2, bool showData=true, bool stopIfTrue=false);
    bool add2ColorScaleRule(const QColor &minColor, const QColor &maxColor, bool stopIfTrue=false);
    bool add3ColorScaleRule(const QColor &minColor, const QColor &midColor, const QColor &maxColor,  bool stopIfTrue=false);

    QList<CellRange> ranges() const;

    void addCell(const CellReference &cell);
    void addCell(int row, int col);
    void addRange(int firstRow, int firstCol, int lastRow, int lastCol);
    void addRange(const CellRange &range);

    //needed by QSharedDataPointer!!
    ConditionalFormatting &operator=(const ConditionalFormatting &other);

private:
    friend class Worksheet;
    friend class ::ConditionalFormattingTest;

private:
    bool saveToXml(QXmlStreamWriter &writer) const;
    bool loadFromXml(QXmlStreamReader &reader, Styles* styles = NULL);

    QSharedDataPointer<ConditionalFormattingPrivate> d;
};

QT_END_NAMESPACE_XLSX

#endif // QXLSX_XLSXCONDITIONALFORMATTING_H
