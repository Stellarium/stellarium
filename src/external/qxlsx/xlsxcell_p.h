//--------------------------------------------------------------------
//
// QXlsx
// MIT License
// https://github.com/j2doll/QXlsx
//
// QtXlsx
// https://github.com/dbzhang800/QtXlsxWriter
// http://qtxlsx.debao.me/
// MIT License


#ifndef XLSXCELL_P_H
#define XLSXCELL_P_H

#include "xlsxglobal.h"
#include "xlsxcell.h"
#include "xlsxcellrange.h"
#include "xlsxrichstring.h"
#include "xlsxcellformula.h"
#include <QtGlobal>
#include <QList>
#include <QSharedPointer>

QT_BEGIN_NAMESPACE_XLSX

class CellPrivate
{
    Q_DECLARE_PUBLIC(Cell)
public:
    CellPrivate(Cell *p);
    CellPrivate(const CellPrivate * const cp);

public:
    QVariant value;
    CellFormula formula;
    Cell::CellType cellType;
    Format format;

    RichString richString;

    Worksheet *parent;
    Cell *q_ptr;

	qint32 styleNumber;
};

QT_END_NAMESPACE_XLSX

#endif // XLSXCELL_P_H
