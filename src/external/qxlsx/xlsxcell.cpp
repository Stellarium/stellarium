// xlsxcell.cpp

#include <cmath>

#include <QtGlobal>
#include <QDebug>
#include <QDateTime>
#include <QDate>
#include <QTime>

#include "xlsxcell.h"
#include "xlsxcell_p.h"
#include "xlsxformat.h"
#include "xlsxformat_p.h"
#include "xlsxutility_p.h"
#include "xlsxworksheet.h"
#include "xlsxworkbook.h"

QT_BEGIN_NAMESPACE_XLSX

CellPrivate::CellPrivate(Cell *p) :
	q_ptr(p)
{

}

CellPrivate::CellPrivate(const CellPrivate * const cp)
    : parent(cp->parent)
    , cellType(cp->cellType)
    , value(cp->value)
    , formula(cp->formula)
    , format(cp->format)
    , richString(cp->richString)
    , styleNumber(cp->styleNumber)
{

}

/*!
  \class Cell
  \inmodule QtXlsx
  \brief The Cell class provides a API that is used to handle the worksheet cell.

*/

/*!
  \enum Cell::CellType
  \value BooleanType      Boolean type
  \value NumberType       Number type, can be blank or used with forumula
  \value ErrorType        Error type
  \value SharedStringType Shared string type
  \value StringType       String type, can be used with forumula
  \value InlineStringType Inline string type
  */

/*!
 * \internal
 * Created by Worksheet only.
 */
// qint32 styleIndex = (-1)
Cell::Cell(const QVariant &data, 
	CellType type, 
	const Format &format, 
	Worksheet *parent,
	qint32 styleIndex ) :
	d_ptr(new CellPrivate(this))
{
	d_ptr->value = data;
	d_ptr->cellType = type;
	d_ptr->format = format;
	d_ptr->parent = parent;
	d_ptr->styleNumber = styleIndex; 
}

/*!
 * \internal
 */
Cell::Cell(const Cell * const cell):
	d_ptr(new CellPrivate(cell->d_ptr))
{
	d_ptr->q_ptr = this;
}

/*!
 * Destroys the Cell and cleans up.
 */
Cell::~Cell()
{
	if ( NULL != d_ptr )
		delete d_ptr;
}

/*!
 * Return the dataType of this Cell
 */
Cell::CellType Cell::cellType() const
{
	Q_D(const Cell);

	return d->cellType;
}

/*!
 * Return the data content of this Cell
 */
QVariant Cell::value() const
{
	Q_D(const Cell); 

	return d->value; 
}

/*!
* Return the data content of this Cell for reading 
*/
QVariant Cell::readValue() const
{
	Q_D(const Cell);

	QVariant ret; // return value 
	ret = d->value;

	Format fmt = this->format();

	if (isDateTime())
	{
        QVariant vDT = dateTime();
        if ( vDT.isNull() )
        {
            return QVariant();
        }

        // https://github.com/QtExcel/QXlsx/issues/171
        // https://www.qt.io/blog/whats-new-in-qmetatype-qvariant
        #if QT_VERSION >= 0x060000 // Qt 6.0 or over
                if ( vDT.metaType().id() == QMetaType::QDateTime )
                {
                    ret = vDT;
                }
                else if ( vDT.metaType().id() == QMetaType::QDate )
                {
                    ret = vDT;
                }
                else if ( vDT.metaType().id() == QMetaType::QTime )
                {
                    ret = vDT;
                }
                else
                {
                    return QVariant();
                }
        #else
                if ( vDT.type() == QVariant::DateTime )
                {
                    ret = vDT;
                }
                else if ( vDT.type() == QVariant::Date )
                {
                    ret = vDT;
                }
                else if ( vDT.type() == QVariant::Time )
                {
                    ret = vDT;
                }
                else
                {
                    return QVariant();
                }
        #endif

        // QDateTime dt = dateTime();
        // ret = dt;
		
        // QString strFormat = fmt.numberFormat();
        // if (!strFormat.isEmpty())
        // {
        // 	// TODO: use number format
        // }

        // qint32 styleNo = d->styleNumber;

        // if (styleNo == 10)
        // {
        // }

        // if (styleNo == 11)
        // {
			// QTime timeValue = dt.time(); // only time. (HH:mm:ss) 
			// ret = timeValue;
			// return ret;
        // }

        // if (styleNo == 12)
        // {
        // }

        // if (styleNo == 13) // (HH:mm:ss)
        // {
            // double dValue = d->value.toDouble();
            // int day = int(dValue); // unit is day.
            // double deciamlPointValue1 = dValue - double(day);

            // double dHour = deciamlPointValue1 * (double(1.0) / double(24.0));
            // int hour = int(dHour);

            // double deciamlPointValue2 = deciamlPointValue1 - (double(hour) * (double(1.0) / double(24.0)));
            // double dMin = deciamlPointValue2 * (double(1.0) / double(60.0));
            // int min = int(dMin);

            // double deciamlPointValue3 = deciamlPointValue2 - (double(min) * (double(1.0) / double(60.0)));
            // double dSec = deciamlPointValue3 * (double(1.0) / double(60.0));
            // int sec = int(dSec);
	
            // int totalHour = hour + (day * 24);

            // QString strTime;
            // strTime = QString("%1:%2:%3").arg(totalHour).arg(min).arg(sec);
            // ret = strTime;

            // return ret;
        // }

        // return ret;
        // */
	}

	if (hasFormula())
	{
		QString formulaString = this->formula().formulaText();
		ret = formulaString;
		return ret; // return formula string 
	}

	return ret;
}

/*!
 * Return the style used by this Cell. If no style used, 0 will be returned.
 */
Format Cell::format() const
{
	Q_D(const Cell);

	return d->format;
}

/*!
 * Returns true if the cell has one formula.
 */
bool Cell::hasFormula() const
{
	Q_D(const Cell);

	return d->formula.isValid();
}

/*!
 * Return the formula contents if the dataType is Formula
 */
CellFormula Cell::formula() const
{
	Q_D(const Cell);

	return d->formula;
}

/*!
 * Returns whether the value is probably a dateTime or not
 */
bool Cell::isDateTime() const
{
	Q_D(const Cell);

	Cell::CellType cellType = d->cellType;
    double dValue = d->value.toDouble(); // number
//	QString strValue = d->value.toString().toUtf8();
	bool isValidFormat = d->format.isValid();
    bool isDateTimeFormat = d->format.isDateTimeFormat(); // datetime format

    // dev67
    if ( cellType == NumberType ||
         cellType == DateType ||
         cellType == CustomType )
    {
        if ( dValue >= 0 &&
             isValidFormat &&
             isDateTimeFormat )
        {
            return true;
        }
    }

	return false;
}

/*!
 * Return the data time value.
 */
/*
QDateTime Cell::dateTime() const
{
	Q_D(const Cell);

	if (!isDateTime())
		return QDateTime();

	return datetimeFromNumber(d->value.toDouble(), d->parent->workbook()->isDate1904());
}
*/
QVariant Cell::dateTime() const
{
    Q_D(const Cell);

    if (!isDateTime())
    {
        return QVariant();
    }

    // dev57

    QVariant ret;
    double dValue = d->value.toDouble();
    bool isDate1904 = d->parent->workbook()->isDate1904();
    ret = datetimeFromNumber(dValue, isDate1904);
    return ret;
}

/*!
 * Returns whether the cell is probably a rich string or not
 */
bool Cell::isRichString() const
{
	Q_D(const Cell);

    if ( d->cellType != SharedStringType &&
            d->cellType != InlineStringType &&
            d->cellType != StringType )
    {
		return false;
    }

	return d->richString.isRichString();
}

qint32 Cell::styleNumber() const 
{
	Q_D(const Cell);

	qint32 ret = d->styleNumber;
	return ret; 
}

QT_END_NAMESPACE_XLSX
