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

#ifndef QXLSX_XLSXCELL_H
#define QXLSX_XLSXCELL_H

#include <QtGlobal>
#include <QVariant>
#include <cstdio>

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

public:
	enum CellType {
		BooleanType,      //t="b"
		NumberType,       //t="n" (default)
		ErrorType,        //t="e"
		SharedStringType, //t="s"
		StringType,       //t="str"
		InlineStringType  //t="inlineStr"
	};
	/* cell type  of MIcrosoft Excel
		01) General − This is the default cell format of Cell.
		02) Number − This displays cell as number with separator.
		03) Currency − This displays cell as currency i.e. with currency sign.
		04) Accounting − Similar to Currency, used for accounting purpose.
		05) Date − Various date formats are available under this like 17-09-2013, 17th-Sep-2013, etc.
		06) Time − Various Time formats are available under this, like 1.30PM, 13.30, etc.
		07) Percentage − This displays cell as percentage with decimal places like 50.00%.
		08) Fraction − This displays cell as fraction like 1/4, 1/2 etc.
		09) Scientific − This displays cell as exponential like 5.6E+01.
		10) Text − This displays cell as normal text.
		11) Special − Special formats of cell like Zip code, Phone Number.
		12) Custom − You can use custom format by using this.
	*/

	CellType cellType() const;
	QVariant value() const;
	QVariant readValue() const;
	Format format() const;
	
	bool hasFormula() const;
	CellFormula formula() const;

	bool isDateTime() const;
	QDateTime dateTime() const;

	bool isRichString() const;

	qint32 styleNumber() const;

	~Cell();
private:
	friend class Worksheet;
	friend class WorksheetPrivate;

	Cell(const QVariant &data = QVariant(), 
		CellType type = NumberType, 
		const Format &format = Format(), 
		Worksheet *parent = NULL,
		qint32 styleIndex = (-1) );
	Cell(const Cell * const cell);
	CellPrivate * const d_ptr;
};

QT_END_NAMESPACE_XLSX

#endif // QXLSX_XLSXCELL_H
