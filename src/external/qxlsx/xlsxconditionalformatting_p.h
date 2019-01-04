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

#ifndef XLSXCONDITIONALFORMATTING_P_H
#define XLSXCONDITIONALFORMATTING_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt Xlsx API.  It exists for the convenience
// of the Qt Xlsx.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include "xlsxconditionalformatting.h"
#include "xlsxformat.h"
#include "xlsxcolor_p.h"
#include <QSharedData>
#include <QSharedPointer>
#include <QMap>

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
