// xlsxchartsheet.h

#ifndef XLSXCHARTSHEET_H
#define XLSXCHARTSHEET_H

#include <QtGlobal>
#include <QStringList>
#include <QSharedPointer>

#include "xlsxabstractsheet.h"

QT_BEGIN_NAMESPACE_XLSX

class Workbook;
class DocumentPrivate;
class ChartsheetPrivate;
class Chart;

class Chartsheet : public AbstractSheet
{
    Q_DECLARE_PRIVATE(Chartsheet)

public:
    ~Chartsheet();
    Chart *chart();

private:
    friend class DocumentPrivate;
    friend class Workbook;

    Chartsheet(const QString &sheetName, int sheetId, Workbook *book, CreateFlag flag);
    Chartsheet *copy(const QString &distName, int distId) const;

    void saveToXmlFile(QIODevice *device) const;
    bool loadFromXmlFile(QIODevice *device);
};

QT_END_NAMESPACE_XLSX
#endif // XLSXCHARTSHEET_H
