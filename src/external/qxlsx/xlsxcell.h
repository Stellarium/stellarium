// xlsxcell.h

#ifndef QXLSX_XLSXCELL_H
#define QXLSX_XLSXCELL_H

#include <cstdio>

#include <QtGlobal>
#include <QObject>
#include <QString>
#include <QVariant>
#include <QDate>
#include <QDateTime>
#include <QTime>

#include "xlsxglobal.h"
#include "xlsxformat.h"

QT_BEGIN_NAMESPACE_XLSX

class Worksheet;
class Format;
class CellFormula;
class CellPrivate;
class WorksheetPrivate;

class Cell
{
	Q_DECLARE_PRIVATE(Cell)

private:
    friend class Worksheet;
    friend class WorksheetPrivate;

public:
    enum CellType // See ECMA 376, 18.18.11. ST_CellType (Cell Type) for more information.
    {
        BooleanType,
        DateType,
        ErrorType,
        InlineStringType,
        NumberType,
        SharedStringType,
        StringType,
        CustomType, // custom or un-defined cell type
	};

public:
    Cell(const QVariant &data = QVariant(),
            CellType type = NumberType,
            const Format &format = Format(),
            Worksheet *parent = NULL,
            qint32 styleIndex = (-1) );
    Cell(const Cell * const cell);
    ~Cell();

public:
    CellPrivate * const d_ptr; // See D-pointer and Q-pointer of Qt, for more information.

public:
	CellType cellType() const;
	QVariant value() const;
	QVariant readValue() const;
	Format format() const;
	
	bool hasFormula() const;
	CellFormula formula() const;

	bool isDateTime() const;
    QVariant dateTime() const; // QDateTime, QDate, QTime

	bool isRichString() const;

	qint32 styleNumber() const;

};

QT_END_NAMESPACE_XLSX

#endif // QXLSX_XLSXCELL_H
