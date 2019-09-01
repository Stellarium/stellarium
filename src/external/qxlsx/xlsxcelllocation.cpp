// xlsx

#include <QtGlobal>
#include <QObject>
#include <QString>
#include <QVector>
#include <QList>

#include "xlsxglobal.h"
#include "xlsxcell.h"
#include "xlsxcelllocation.h"

QT_BEGIN_NAMESPACE_XLSX

CellLocation::CellLocation()
{
    col = -1;
    row = -1;

    cell.clear();
}

QT_END_NAMESPACE_XLSX
