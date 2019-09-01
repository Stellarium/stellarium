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
#ifndef XLSXABSTRACTSHEET_H
#define XLSXABSTRACTSHEET_H

#include "xlsxabstractooxmlfile.h"
#include <QStringList>
#include <QSharedPointer>

QT_BEGIN_NAMESPACE_XLSX
class Workbook;
class Drawing;
class AbstractSheetPrivate;
class AbstractSheet : public AbstractOOXmlFile
{
    Q_DECLARE_PRIVATE(AbstractSheet)
public:
    enum SheetType {
        ST_WorkSheet,
        ST_ChartSheet,
        ST_DialogSheet,
        ST_MacroSheet
    };

    enum SheetState {
        SS_Visible,
        SS_Hidden,
        SS_VeryHidden
    };

    QString sheetName() const;
    SheetType sheetType() const;
    SheetState sheetState() const;
    void setSheetState(SheetState ss);
    bool isHidden() const;
    bool isVisible() const;
    void setHidden(bool hidden);
    void setVisible(bool visible);

    Workbook *workbook() const;

protected:
    friend class Workbook;
    AbstractSheet(const QString &sheetName, int sheetId, Workbook *book, AbstractSheetPrivate *d);
    virtual AbstractSheet *copy(const QString &distName, int distId) const = 0;
    void setSheetName(const QString &sheetName);
    void setSheetType(SheetType type);
    int sheetId() const;

    Drawing *drawing() const;
};

QT_END_NAMESPACE_XLSX
#endif // XLSXABSTRACTSHEET_H
