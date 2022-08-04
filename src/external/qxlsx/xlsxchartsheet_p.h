// xlsxchartsheet_p.h

#ifndef XLSXCHARTSHEET_P_H
#define XLSXCHARTSHEET_P_H

#include <QtGlobal>

#include "xlsxglobal.h"
#include "xlsxchartsheet.h"
#include "xlsxabstractsheet_p.h"

QT_BEGIN_NAMESPACE_XLSX

class ChartsheetPrivate : public AbstractSheetPrivate
{
    Q_DECLARE_PUBLIC(Chartsheet)
public:
    ChartsheetPrivate(Chartsheet *p, Chartsheet::CreateFlag flag);
    ~ChartsheetPrivate();

    Chart *chart;
};

QT_END_NAMESPACE_XLSX
#endif // XLSXCHARTSHEET_P_H
