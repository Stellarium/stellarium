// xlsxabstractsheet.h

#ifndef XLSXABSTRACTSHEET_H
#define XLSXABSTRACTSHEET_H

#include "xlsxglobal.h"
#include "xlsxabstractooxmlfile.h"

QT_BEGIN_NAMESPACE_XLSX

class Workbook;
class Drawing;
class AbstractSheetPrivate;

class AbstractSheet : public AbstractOOXmlFile
{
    Q_DECLARE_PRIVATE(AbstractSheet)

public:
    Workbook *workbook() const;

public:
    // NOTE: If all Qt  compiler supports C++1x, recommend to use a 'class enum'.
    enum SheetType { ST_WorkSheet, ST_ChartSheet, ST_DialogSheet, ST_MacroSheet };
    enum SheetState { SS_Visible,SS_Hidden, SS_VeryHidden };

public:
    QString sheetName() const;
    SheetType sheetType() const;
    SheetState sheetState() const;
    void setSheetState(SheetState ss);
    bool isHidden() const;
    bool isVisible() const;
    void setHidden(bool hidden);
    void setVisible(bool visible);

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
