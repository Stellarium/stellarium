// xlsxconditionalformatting_p.h

#ifndef XLSXCONDITIONALFORMATTING_P_H
#define XLSXCONDITIONALFORMATTING_P_H

#include <QtGlobal>
#include <QSharedData>
#include <QSharedPointer>
#include <QMap>

#include "xlsxconditionalformatting.h"
#include "xlsxformat.h"
#include "xlsxcolor_p.h"

QT_BEGIN_NAMESPACE_XLSX

class XlsxCfVoData
{
public:
    XlsxCfVoData()
        :gte(true)
    {
    }

    XlsxCfVoData(ConditionalFormatting::ValueObjectType type, const QString &value, bool gte=true)
        :type(type), value(value), gte(gte)
    {
    }

    ConditionalFormatting::ValueObjectType type;
    QString value;
    bool gte;
};

class XlsxCfRuleData
{
public:
    enum Attribute {
        A_type,
        A_dxfId,
        //A_priority,
        A_stopIfTrue,
        A_aboveAverage,
        A_percent,
        A_bottom,
        A_operator,
        A_text,
        A_timePeriod,
        A_rank,
        A_stdDev,
        A_equalAverage,

        A_dxfFormat,
        A_formula1,
        A_formula2,
        A_formula3,
        A_formula1_temp,

        A_color1,
        A_color2,
        A_color3,

        A_cfvo1,
        A_cfvo2,
        A_cfvo3,

        A_hideData
    };

    XlsxCfRuleData()
        :priority(1)
    {}

    int priority;
    Format dxfFormat;
    QMap<int, QVariant> attrs;
};

class ConditionalFormattingPrivate : public QSharedData
{
public:
    ConditionalFormattingPrivate();
    ConditionalFormattingPrivate(const ConditionalFormattingPrivate &other);
    ~ConditionalFormattingPrivate();

    void writeCfVo(QXmlStreamWriter &writer, const XlsxCfVoData& cfvo) const;
    bool readCfVo(QXmlStreamReader &reader, XlsxCfVoData& cfvo);
    bool readCfRule(QXmlStreamReader &reader, XlsxCfRuleData *cfRule, Styles *styles);
    bool readCfDataBar(QXmlStreamReader &reader, XlsxCfRuleData *cfRule);
    bool readCfColorScale(QXmlStreamReader &reader, XlsxCfRuleData *cfRule);

    QList<QSharedPointer<XlsxCfRuleData> >cfRules;
    QList<CellRange> ranges;
};

QT_END_NAMESPACE_XLSX

Q_DECLARE_METATYPE(QXlsx::XlsxCfVoData)
#endif // XLSXCONDITIONALFORMATTING_P_H
