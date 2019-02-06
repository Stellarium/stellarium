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
//--------------------------------------------------------------------

#include <QtGlobal>
#include <QVariant>
#include <QDateTime>
#include <QPoint>
#include <QFile>
#include <QUrl>
#include <QRegularExpression>
#include <QDebug>
#include <QBuffer>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>
#include <QTextDocument>
#include <QDir>
#include <QMapIterator>

#include <cmath>

#include "xlsxrichstring.h"
#include "xlsxcellreference.h"
#include "xlsxworksheet.h"
#include "xlsxworksheet_p.h"
#include "xlsxworkbook.h"
#include "xlsxformat.h"
#include "xlsxformat_p.h"
#include "xlsxutility_p.h"
#include "xlsxsharedstrings_p.h"
#include "xlsxdrawing_p.h"
#include "xlsxstyles_p.h"
#include "xlsxcell.h"
#include "xlsxcell_p.h"
#include "xlsxcellrange.h"
#include "xlsxconditionalformatting_p.h"
#include "xlsxdrawinganchor_p.h"
#include "xlsxchart.h"
#include "xlsxcellformula.h"
#include "xlsxcellformula_p.h"
#include "xlsxcelllocation.h"

QT_BEGIN_NAMESPACE_XLSX

WorksheetPrivate::WorksheetPrivate(Worksheet *p, Worksheet::CreateFlag flag)
	: AbstractSheetPrivate(p, flag)
  , windowProtection(false), showFormulas(false), showGridLines(true), showRowColHeaders(true)
  , showZeros(true), rightToLeft(false), tabSelected(false), showRuler(false)
  , showOutlineSymbols(true), showWhiteSpace(true), urlPattern(QStringLiteral("^([fh]tt?ps?://)|(mailto:)|(file://)"))
{
	previous_row = 0;

	outline_row_level = 0;
	outline_col_level = 0;

	default_row_height = 15;
	default_row_zeroed = false;
}

WorksheetPrivate::~WorksheetPrivate()
{
}

/*
  Calculate the "spans" attribute of the <row> tag. This is an
  XLSX optimisation and isn't strictly required. However, it
  makes comparing files easier. The span is the same for each
  block of 16 rows.
 */
void WorksheetPrivate::calculateSpans() const
{
	row_spans.clear();
	int span_min = XLSX_COLUMN_MAX+1;
	int span_max = -1;

	for (int row_num = dimension.firstRow(); row_num <= dimension.lastRow(); row_num++) {
		if (cellTable.contains(row_num)) {
			for (int col_num = dimension.firstColumn(); col_num <= dimension.lastColumn(); col_num++) {
				if (cellTable[row_num].contains(col_num)) {
					if (span_max == -1) {
						span_min = col_num;
						span_max = col_num;
					} else {
						if (col_num < span_min)
							span_min = col_num;
						else if (col_num > span_max)
							span_max = col_num;
					}
				}
			}
		}
		if (comments.contains(row_num)) {
			for (int col_num = dimension.firstColumn(); col_num <= dimension.lastColumn(); col_num++) {
				if (comments[row_num].contains(col_num)) {
					if (span_max == -1) {
						span_min = col_num;
						span_max = col_num;
					} else {
						if (col_num < span_min)
							span_min = col_num;
						else if (col_num > span_max)
							span_max = col_num;
					}
				}
			}
		}

		if (row_num%16 == 0 || row_num == dimension.lastRow()) {
			if (span_max != -1) {
				row_spans[row_num / 16] = QStringLiteral("%1:%2").arg(span_min).arg(span_max);
				span_min = XLSX_COLUMN_MAX+1;
				span_max = -1;
			}
		}
	}
}


QString WorksheetPrivate::generateDimensionString() const
{
	if (!dimension.isValid())
		return QStringLiteral("A1");
	else
		return dimension.toString();
}

/*
  Check that row and col are valid and store the max and min
  values for use in other methods/elements. The ignore_row /
  ignore_col flags is used to indicate that we wish to perform
  the dimension check without storing the value. The ignore
  flags are use by setRow() and dataValidate.
*/
int WorksheetPrivate::checkDimensions(int row, int col, bool ignore_row, bool ignore_col)
{
	Q_ASSERT_X(row!=0, "checkDimensions", "row should start from 1 instead of 0");
	Q_ASSERT_X(col!=0, "checkDimensions", "column should start from 1 instead of 0");

	if (row > XLSX_ROW_MAX || row < 1 || col > XLSX_COLUMN_MAX || col < 1)
		return -1;

	if (!ignore_row) {
		if (row < dimension.firstRow() || dimension.firstRow() == -1) dimension.setFirstRow(row);
		if (row > dimension.lastRow()) dimension.setLastRow(row);
	}
	if (!ignore_col) {
		if (col < dimension.firstColumn() || dimension.firstColumn() == -1) dimension.setFirstColumn(col);
		if (col > dimension.lastColumn()) dimension.setLastColumn(col);
	}

	return 0;
}

/*!
  \class Worksheet
  \inmodule QtXlsx
  \brief Represent one worksheet in the workbook.
*/

/*!
 * \internal
 */
Worksheet::Worksheet(const QString &name, int id, Workbook *workbook, CreateFlag flag)
	:AbstractSheet(name, id, workbook, new WorksheetPrivate(this, flag))
{
	if (!workbook) //For unit test propose only. Ignore the memery leak.
		d_func()->workbook = new Workbook(flag);
}

/*!
 * \internal
 *
 * Make a copy of this sheet.
 */

Worksheet *Worksheet::copy(const QString &distName, int distId) const
{
	Q_D(const Worksheet);
	Worksheet *sheet = new Worksheet(distName, distId, d->workbook, F_NewFromScratch);
	WorksheetPrivate *sheet_d = sheet->d_func();

	sheet_d->dimension = d->dimension;

	QMapIterator<int, QMap<int, QSharedPointer<Cell> > > it(d->cellTable);
	while (it.hasNext()) {
		it.next();
		int row = it.key();
		QMapIterator<int, QSharedPointer<Cell> > it2(it.value());
		while (it2.hasNext()) {
			it2.next();
			int col = it2.key();

			QSharedPointer<Cell> cell(new Cell(it2.value().data()));
			cell->d_ptr->parent = sheet;

			if (cell->cellType() == Cell::SharedStringType)
				d->workbook->sharedStrings()->addSharedString(cell->d_ptr->richString);

			sheet_d->cellTable[row][col] = cell;
		}
	}

	sheet_d->merges = d->merges;
//    sheet_d->rowsInfo = d->rowsInfo;
//    sheet_d->colsInfo = d->colsInfo;
//    sheet_d->colsInfoHelper = d->colsInfoHelper;
//    sheet_d->dataValidationsList = d->dataValidationsList;
//    sheet_d->conditionalFormattingList = d->conditionalFormattingList;

	return sheet;
}

/*!
 * Destroys this workssheet.
 */
Worksheet::~Worksheet()
{
}

/*!
 * Returns whether sheet is protected.
 */
bool Worksheet::isWindowProtected() const
{
	Q_D(const Worksheet);
	return d->windowProtection;
}

/*!
 * Protects/unprotects the sheet based on \a protect.
 */
void Worksheet::setWindowProtected(bool protect)
{
	Q_D(Worksheet);
	d->windowProtection = protect;
}

/*!
 * Return whether formulas instead of their calculated results shown in cells
 */
bool Worksheet::isFormulasVisible() const
{
	Q_D(const Worksheet);
	return d->showFormulas;
}

/*!
 * Show formulas in cells instead of their calculated results when \a visible is true.
 */
void Worksheet::setFormulasVisible(bool visible)
{
	Q_D(Worksheet);
	d->showFormulas = visible;
}

/*!
 * Return whether gridlines is shown or not.
 */
bool Worksheet::isGridLinesVisible() const
{
	Q_D(const Worksheet);
	return d->showGridLines;
}

/*!
 * Show or hide the gridline based on \a visible
 */
void Worksheet::setGridLinesVisible(bool visible)
{
	Q_D(Worksheet);
	d->showGridLines = visible;
}

/*!
 * Return whether is row and column headers is vislbe.
 */
bool Worksheet::isRowColumnHeadersVisible() const
{
	Q_D(const Worksheet);
	return d->showRowColHeaders;
}

/*!
 * Show or hide the row column headers based on \a visible
 */
void Worksheet::setRowColumnHeadersVisible(bool visible)
{
	Q_D(Worksheet);
	d->showRowColHeaders = visible;
}


/*!
 * Return whether the sheet is shown right-to-left or not.
 */
bool Worksheet::isRightToLeft() const
{
	Q_D(const Worksheet);
	return d->rightToLeft;
}

/*!
 * Enable or disable the right-to-left based on \a enable.
 */
void Worksheet::setRightToLeft(bool enable)
{
	Q_D(Worksheet);
	d->rightToLeft = enable;
}

/*!
 * Return whether is cells that have zero value show a zero.
 */
bool Worksheet::isZerosVisible() const
{
	Q_D(const Worksheet);
	return d->showZeros;
}

/*!
 * Show a zero in cells that have zero value if \a visible is true.
 */
void Worksheet::setZerosVisible(bool visible)
{
	Q_D(Worksheet);
	d->showZeros = visible;
}

/*!
 * Return whether this tab is selected.
 */
bool Worksheet::isSelected() const
{
	Q_D(const Worksheet);
	return d->tabSelected;
}

/*!
 * Select this sheet if \a select is true.
 */
void Worksheet::setSelected(bool select)
{
	Q_D(Worksheet);
	d->tabSelected = select;
}

/*!
 * Return whether is ruler is shown.
 */
bool Worksheet::isRulerVisible() const
{
	Q_D(const Worksheet);
	return d->showRuler;
}

/*!
 * Show or hide the ruler based on \a visible.
 */
void Worksheet::setRulerVisible(bool visible)
{
	Q_D(Worksheet);
	d->showRuler = visible;
}

/*!
 * Return whether is outline symbols is shown.
 */
bool Worksheet::isOutlineSymbolsVisible() const
{
	Q_D(const Worksheet);
	return d->showOutlineSymbols;
}

/*!
 * Show or hide the outline symbols based ib \a visible.
 */
void Worksheet::setOutlineSymbolsVisible(bool visible)
{
	Q_D(Worksheet);
	d->showOutlineSymbols = visible;
}

/*!
 * Return whether is white space is shown.
 */
bool Worksheet::isWhiteSpaceVisible() const
{
	Q_D(const Worksheet);
	return d->showWhiteSpace;
}

/*!
 * Show or hide the white space based on \a visible.
 */
void Worksheet::setWhiteSpaceVisible(bool visible)
{
	Q_D(Worksheet);
	d->showWhiteSpace = visible;
}

/*!
 * Write \a value to cell (\a row, \a column) with the \a format.
 * Both \a row and \a column are all 1-indexed value.
 *
 * Returns true on success.
 */
bool Worksheet::write(int row, int column, const QVariant &value, const Format &format)
{
	Q_D(Worksheet);

	if (d->checkDimensions(row, column))
		return false;

	bool ret = true;
	if (value.isNull()) {
		//Blank
		ret = writeBlank(row, column, format);
	} else if (value.userType() == QMetaType::QString) {
		//String
		QString token = value.toString();
		bool ok;

		if (token.startsWith(QLatin1String("="))) {
			//convert to formula
			ret = writeFormula(row, column, CellFormula(token), format);
		} else if (d->workbook->isStringsToHyperlinksEnabled() && token.contains(d->urlPattern)) {
			//convert to url
			ret = writeHyperlink(row, column, QUrl(token));
		} else if (d->workbook->isStringsToNumbersEnabled() && (value.toDouble(&ok), ok)) {
			//Try convert string to number if the flag enabled.
			ret = writeString(row, column, value.toString(), format);
		} else {
			//normal string now
			ret = writeString(row, column, token, format);
		}
	} else if (value.userType() == qMetaTypeId<RichString>()) {
		ret = writeString(row, column, value.value<RichString>(), format);
	} else if (value.userType() == QMetaType::Int || value.userType() == QMetaType::UInt
			   || value.userType() == QMetaType::LongLong || value.userType() == QMetaType::ULongLong
			   || value.userType() == QMetaType::Double || value.userType() == QMetaType::Float) {
		//Number

		ret = writeNumeric(row, column, value.toDouble(), format);
	} else if (value.userType() == QMetaType::Bool) {
		//Bool
		ret = writeBool(row,column, value.toBool(), format);
	} else if (value.userType() == QMetaType::QDateTime || value.userType() == QMetaType::QDate) {
		//DateTime, Date
		//  note that, QTime cann't convert to QDateTime
		ret = writeDateTime(row, column, value.toDateTime(), format);
	} else if (value.userType() == QMetaType::QTime) {
		//Time
		ret = writeTime(row, column, value.toTime(), format);
	} else if (value.userType() == QMetaType::QUrl) {
		//Url
		ret = writeHyperlink(row, column, value.toUrl(), format);
	} else {
		//Wrong type
		return false;
	}

	return ret;
}

/*!
 * \overload
 * Write \a value to cell \a row_column with the \a format.
 * Both row and column are all 1-indexed value.
 * Returns true on success.
 */
bool Worksheet::write(const CellReference &row_column, const QVariant &value, const Format &format)
{
	if (!row_column.isValid())
		return false;

	return write(row_column.row(), row_column.column(), value, format);
}

/*!
	\overload
	Return the contents of the cell \a row_column.
 */
QVariant Worksheet::read(const CellReference &row_column) const
{
	if (!row_column.isValid())
		return QVariant();

	return read(row_column.row(), row_column.column());
}

/*!
	Return the contents of the cell (\a row, \a column).
 */
QVariant Worksheet::read(int row, int column) const
{
	Q_D(const Worksheet);

	Cell *cell = cellAt(row, column);
	if (!cell)
		return QVariant();

	if (cell->hasFormula()) {
		if (cell->formula().formulaType() == CellFormula::NormalType) {
			return QVariant(QLatin1String("=")+cell->formula().formulaText());
		} else if (cell->formula().formulaType() == CellFormula::SharedType) {
			if (!cell->formula().formulaText().isEmpty()) {
				return QVariant(QLatin1String("=")+cell->formula().formulaText());
			} else {
				const CellFormula &rootFormula = d->sharedFormulaMap[cell->formula().sharedIndex()];
				CellReference rootCellRef = rootFormula.reference().topLeft();
				QString rootFormulaText = rootFormula.formulaText();
				QString newFormulaText = convertSharedFormula(rootFormulaText, rootCellRef, CellReference(row, column));
				return QVariant(QLatin1String("=")+newFormulaText);
			}
		}
	}

	if (cell->isDateTime()) {
		double val = cell->value().toDouble();
		QDateTime dt = cell->dateTime();
		if (val < 1)
			return dt.time();
		if (fmod(val, 1.0) <  1.0/(1000*60*60*24)) //integer
			return dt.date();
		return dt;
	}

	return cell->value();
}

/*!
 * Returns the cell at the given \a row_column. If there
 * is no cell at the specified position, the function returns 0.
 */
Cell *Worksheet::cellAt(const CellReference &row_column) const
{
	if (!row_column.isValid())
		return 0;

	return cellAt(row_column.row(), row_column.column());
}

/*!
 * Returns the cell at the given \a row and \a column. If there
 * is no cell at the specified position, the function returns 0.
 */
Cell *Worksheet::cellAt(int row, int column) const
{
	Q_D(const Worksheet);
	if (!d->cellTable.contains(row))
		return 0;
	if (!d->cellTable[row].contains(column))
		return 0;

	return d->cellTable[row][column].data();
}

Format WorksheetPrivate::cellFormat(int row, int col) const
{
	if (!cellTable.contains(row))
		return Format();
	if (!cellTable[row].contains(col))
		return Format();
	return cellTable[row][col]->format();
}

/*!
  \overload
  Write string \a value to the cell \a row_column with the \a format.

  Returns true on success.
 */
bool Worksheet::writeString(const CellReference &row_column, const RichString &value, const Format &format)
{
	if (!row_column.isValid())
		return false;

	return writeString(row_column.row(), row_column.column(), value, format);
}

/*!
  Write string \a value to the cell (\a row, \a column) with the \a format.
  Returns true on success.
*/
bool Worksheet::writeString(int row, int column, const RichString &value, const Format &format)
{
	Q_D(Worksheet);
//    QString content = value.toPlainString();
	if (d->checkDimensions(row, column))
		return false;

//    if (content.size() > d->xls_strmax) {
//        content = content.left(d->xls_strmax);
//        error = -2;
//    }

	d->sharedStrings()->addSharedString(value);
	Format fmt = format.isValid() ? format : d->cellFormat(row, column);
	if (value.fragmentCount() == 1 && value.fragmentFormat(0).isValid())
		fmt.mergeFormat(value.fragmentFormat(0));
	d->workbook->styles()->addXfFormat(fmt);
	QSharedPointer<Cell> cell = QSharedPointer<Cell>(new Cell(value.toPlainString(), Cell::SharedStringType, fmt, this));
	cell->d_ptr->richString = value;
	d->cellTable[row][column] = cell;
	return true;
}

/*!
	\overload
	Write string \a value to the cell \a row_column with the \a format.
 */
bool Worksheet::writeString(const CellReference &row_column, const QString &value, const Format &format)
{
	if (!row_column.isValid())
		return false;

	return writeString(row_column.row(), row_column.column(), value, format);
}

/*!
	\overload

	Write string \a value to the cell (\a row, \a column) with the \a format.
	Returns true on success.
*/
bool Worksheet::writeString(int row, int column, const QString &value, const Format &format)
{
	Q_D(Worksheet);
	if (d->checkDimensions(row, column))
		return false;

	RichString rs;
	if (d->workbook->isHtmlToRichStringEnabled() && Qt::mightBeRichText(value))
		rs.setHtml(value);
	else
		rs.addFragment(value, Format());

	return writeString(row, column, rs, format);
}

/*!
	\overload
	Write string \a value to the cell \a row_column with the \a format
 */
bool Worksheet::writeInlineString(const CellReference &row_column, const QString &value, const Format &format)
{
	if (!row_column.isValid())
		return false;

	return writeInlineString(row_column.row(), row_column.column(), value, format);
}

/*!
	Write string \a value to the cell (\a row, \a column) with the \a format.
	Returns true on success.
*/
bool Worksheet::writeInlineString(int row, int column, const QString &value, const Format &format)
{
	Q_D(Worksheet);
	//int error = 0;
	QString content = value;
	if (d->checkDimensions(row, column))
		return false;

	if (value.size() > XLSX_STRING_MAX) {
		content = value.left(XLSX_STRING_MAX);
		//error = -2;
	}

	Format fmt = format.isValid() ? format : d->cellFormat(row, column);
	d->workbook->styles()->addXfFormat(fmt);
	d->cellTable[row][column] = QSharedPointer<Cell>(new Cell(value, Cell::InlineStringType, fmt, this));
	return true;
}

/*!
	\overload
	Write numeric \a value to the cell \a row_column with the \a format.
	Returns true on success.
 */
bool Worksheet::writeNumeric(const CellReference &row_column, double value, const Format &format)
{
	if (!row_column.isValid())
		return false;

	return writeNumeric(row_column.row(), row_column.column(), value, format);
}

/*!
	Write numeric \a value to the cell (\a row, \a column) with the \a format.
	Returns true on success.
*/
bool Worksheet::writeNumeric(int row, int column, double value, const Format &format)
{
	Q_D(Worksheet);
	if (d->checkDimensions(row, column))
		return false;

	Format fmt = format.isValid() ? format : d->cellFormat(row, column);
	d->workbook->styles()->addXfFormat(fmt);
	d->cellTable[row][column] = QSharedPointer<Cell>(new Cell(value, Cell::NumberType, fmt, this));
	return true;
}

/*!
	\overload
	Write \a formula to the cell \a row_column with the \a format and \a result.
	Returns true on success.
 */
bool Worksheet::writeFormula(const CellReference &row_column, const CellFormula &formula, const Format &format, double result)
{
	if (!row_column.isValid())
		return false;

	return writeFormula(row_column.row(), row_column.column(), formula, format, result);
}

/*!
	Write \a formula_ to the cell (\a row, \a column) with the \a format and \a result.
	Returns true on success.
*/
bool Worksheet::writeFormula(int row, int column, const CellFormula &formula_, const Format &format, double result)
{
	Q_D(Worksheet);
	if (d->checkDimensions(row, column))
		return false;

	Format fmt = format.isValid() ? format : d->cellFormat(row, column);
	d->workbook->styles()->addXfFormat(fmt);

	CellFormula formula = formula_;
	formula.d->ca = true;
	if (formula.formulaType() == CellFormula::SharedType) {
		//Assign proper shared index for shared formula
		int si=0;
		while(d->sharedFormulaMap.contains(si))
			++si;
		formula.d->si = si;
		d->sharedFormulaMap[si] = formula;
	}

	QSharedPointer<Cell> data = QSharedPointer<Cell>(new Cell(result, Cell::NumberType, fmt, this));
	data->d_ptr->formula = formula;
	d->cellTable[row][column] = data;

	CellRange range = formula.reference();
	if (formula.formulaType() == CellFormula::SharedType) {
		CellFormula sf(QString(), CellFormula::SharedType);
		sf.d->si = formula.sharedIndex();
		for (int r=range.firstRow(); r<=range.lastRow(); ++r) {
			for (int c=range.firstColumn(); c<=range.lastColumn(); ++c) {
				if (!(r==row && c==column)) {
					if(Cell *cell = cellAt(r, c)) {
						cell->d_ptr->formula = sf;
					} else {
						QSharedPointer<Cell> newCell = QSharedPointer<Cell>(new Cell(result, Cell::NumberType, fmt, this));
						newCell->d_ptr->formula = sf;
						d->cellTable[r][c] = newCell;
					}
				}
			}
		}
	} else if (formula.formulaType() == CellFormula::SharedType) {
	}

	return true;
}

/*!
	\overload
	Write a empty cell \a row_column with the \a format.
	Returns true on success.
 */
bool Worksheet::writeBlank(const CellReference &row_column, const Format &format)
{
	if (!row_column.isValid())
		return false;

	return writeBlank(row_column.row(), row_column.column(), format);
}

/*!
	Write a empty cell (\a row, \a column) with the \a format.
	Returns true on success.
 */
bool Worksheet::writeBlank(int row, int column, const Format &format)
{
	Q_D(Worksheet);
	if (d->checkDimensions(row, column))
		return false;

	Format fmt = format.isValid() ? format : d->cellFormat(row, column);
	d->workbook->styles()->addXfFormat(fmt);

	//Note: NumberType with an invalid QVariant value means blank.
	d->cellTable[row][column] = QSharedPointer<Cell>(new Cell(QVariant(), Cell::NumberType, fmt, this));

	return true;
}
/*!
	\overload
	Write a bool \a value to the cell \a row_column with the \a format.
	Returns true on success.
 */
bool Worksheet::writeBool(const CellReference &row_column, bool value, const Format &format)
{
	if (!row_column.isValid())
		return false;

	return writeBool(row_column.row(), row_column.column(), value, format);
}

/*!
	Write a bool \a value to the cell (\a row, \a column) with the \a format.
	Returns true on success.
 */
bool Worksheet::writeBool(int row, int column, bool value, const Format &format)
{
	Q_D(Worksheet);
	if (d->checkDimensions(row, column))
		return false;

	Format fmt = format.isValid() ? format : d->cellFormat(row, column);
	d->workbook->styles()->addXfFormat(fmt);
	d->cellTable[row][column] = QSharedPointer<Cell>(new Cell(value, Cell::BooleanType, fmt, this));

	return true;
}
/*!
	\overload
	Write a QDateTime \a dt to the cell \a row_column with the \a format.
	Returns true on success.
 */
bool Worksheet::writeDateTime(const CellReference &row_column, const QDateTime &dt, const Format &format)
{
	if (!row_column.isValid())
		return false;

	return writeDateTime(row_column.row(), row_column.column(), dt, format);
}

/*!
	Write a QDateTime \a dt to the cell (\a row, \a column) with the \a format.
	Returns true on success.
 */
bool Worksheet::writeDateTime(int row, int column, const QDateTime &dt, const Format &format)
{
	Q_D(Worksheet);
	if (d->checkDimensions(row, column))
		return false;

	Format fmt = format.isValid() ? format : d->cellFormat(row, column);
	if (!fmt.isValid() || !fmt.isDateTimeFormat())
		fmt.setNumberFormat(d->workbook->defaultDateFormat());
	d->workbook->styles()->addXfFormat(fmt);

	double value = datetimeToNumber(dt, d->workbook->isDate1904());

	d->cellTable[row][column] = QSharedPointer<Cell>(new Cell(value, Cell::NumberType, fmt, this));

	return true;
}

/*!
	\overload
	Write a QTime \a t to the cell \a row_column with the \a format.
	Returns true on success.
 */
bool Worksheet::writeTime(const CellReference &row_column, const QTime &t, const Format &format)
{
	if (!row_column.isValid())
		return false;

	return writeTime(row_column.row(), row_column.column(), t, format);
}

/*!
	Write a QTime \a t to the cell (\a row, \a column) with the \a format.
	Returns true on success.
 */
bool Worksheet::writeTime(int row, int column, const QTime &t, const Format &format)
{
	Q_D(Worksheet);
	if (d->checkDimensions(row, column))
		return false;

	Format fmt = format.isValid() ? format : d->cellFormat(row, column);
	if (!fmt.isValid() || !fmt.isDateTimeFormat())
		fmt.setNumberFormat(QStringLiteral("hh:mm:ss"));
	d->workbook->styles()->addXfFormat(fmt);

	d->cellTable[row][column] = QSharedPointer<Cell>(new Cell(timeToNumber(t), Cell::NumberType, fmt, this));

	return true;
}

/*!
	\overload
	Write a QUrl \a url to the cell \a row_column with the given \a format \a display and \a tip.
	Returns true on success.
 */
bool Worksheet::writeHyperlink(const CellReference &row_column, const QUrl &url, const Format &format, const QString &display, const QString &tip)
{
	if (!row_column.isValid())
		return false;

	return writeHyperlink(row_column.row(), row_column.column(), url, format, display, tip);
}

/*!
	Write a QUrl \a url to the cell (\a row, \a column) with the given \a format \a display and \a tip.
	Returns true on success.
 */
bool Worksheet::writeHyperlink(int row, int column, const QUrl &url, const Format &format, const QString &display, const QString &tip)
{
	Q_D(Worksheet);
	if (d->checkDimensions(row, column))
		return false;

	//int error = 0;

	QString urlString = url.toString();

	//Generate proper display string
	QString displayString = display.isEmpty() ? urlString : display;
	if (displayString.startsWith(QLatin1String("mailto:")))
		displayString.replace(QLatin1String("mailto:"), QString());
	if (displayString.size() > XLSX_STRING_MAX) {
		displayString = displayString.left(XLSX_STRING_MAX);
		//error = -2;
	}

	/*
	  Location within target. If target is a workbook (or this workbook)
	  this shall refer to a sheet and cell or a defined name. Can also
	  be an HTML anchor if target is HTML file.

	  c:\temp\file.xlsx#Sheet!A1
	  http://a.com/aaa.html#aaaaa
	*/
	QString locationString;
	if (url.hasFragment()) {
		locationString = url.fragment();
		urlString = url.toString(QUrl::RemoveFragment);
	}

	Format fmt = format.isValid() ? format : d->cellFormat(row, column);
	//Given a default style for hyperlink
	if (!fmt.isValid()) {
		fmt.setFontColor(Qt::blue);
		fmt.setFontUnderline(Format::FontUnderlineSingle);
	}
	d->workbook->styles()->addXfFormat(fmt);

	//Write the hyperlink string as normal string.
	d->sharedStrings()->addSharedString(displayString);
	d->cellTable[row][column] = QSharedPointer<Cell>(new Cell(displayString, Cell::SharedStringType, fmt, this));

	//Store the hyperlink data in a separate table
	d->urlTable[row][column] = QSharedPointer<XlsxHyperlinkData>(new XlsxHyperlinkData(XlsxHyperlinkData::External, urlString, locationString, QString(), tip));

	return true;
}

/*!
 * Add one DataValidation \a validation to the sheet.
 * Returns true on success.
 */
bool Worksheet::addDataValidation(const DataValidation &validation)
{
	Q_D(Worksheet);
	if (validation.ranges().isEmpty() || validation.validationType()==DataValidation::None)
		return false;

	d->dataValidationsList.append(validation);
	return true;
}

/*!
 * Add one ConditionalFormatting \a cf to the sheet.
 * Returns true on success.
 */
bool Worksheet::addConditionalFormatting(const ConditionalFormatting &cf)
{
	Q_D(Worksheet);
	if (cf.ranges().isEmpty())
		return false;

	for (int i=0; i<cf.d->cfRules.size(); ++i) {
		const QSharedPointer<XlsxCfRuleData> &rule = cf.d->cfRules[i];
		if (!rule->dxfFormat.isEmpty())
			d->workbook->styles()->addDxfFormat(rule->dxfFormat);
		rule->priority = 1;
	}
	d->conditionalFormattingList.append(cf);
	return true;
}

/*!
 * Insert an \a image  at the position \a row, \a column
 * Returns true on success.
 */
bool Worksheet::insertImage(int row, int column, const QImage &image)
{
	Q_D(Worksheet);

	if (image.isNull())
		return false;

	if (!d->drawing)
		d->drawing = QSharedPointer<Drawing>(new Drawing(this, F_NewFromScratch));

	DrawingOneCellAnchor *anchor = new DrawingOneCellAnchor(d->drawing.data(), DrawingAnchor::Picture);

	/*
		The size are expressed as English Metric Units (EMUs). There are
		12,700 EMUs per point. Therefore, 12,700 * 3 /4 = 9,525 EMUs per
		pixel
	*/
	anchor->from = XlsxMarker(row, column, 0, 0);
	anchor->ext = QSize(image.width() * 9525, image.height() * 9525);

	anchor->setObjectPicture(image);
	return true;
}

/*!
 * Creates an chart with the given \a size and insert
 * at the position \a row, \a column.
 * The chart will be returned.
 */
Chart *Worksheet::insertChart(int row, int column, const QSize &size)
{
	Q_D(Worksheet);

	if (!d->drawing)
		d->drawing = QSharedPointer<Drawing>(new Drawing(this, F_NewFromScratch));

	DrawingOneCellAnchor *anchor = new DrawingOneCellAnchor(d->drawing.data(), DrawingAnchor::Picture);

	/*
		The size are expressed as English Metric Units (EMUs). There are
		12,700 EMUs per point. Therefore, 12,700 * 3 /4 = 9,525 EMUs per
		pixel
	*/
	anchor->from = XlsxMarker(row, column, 0, 0);
	anchor->ext = size * 9525;

	QSharedPointer<Chart> chart = QSharedPointer<Chart>(new Chart(this, F_NewFromScratch));
	anchor->setObjectGraphicFrame(chart);

	return chart.data();
}

/*!
	Merge a \a range of cells. The first cell should contain the data and the others should
	be blank. All cells will be applied the same style if a valid \a format is given.
	Returns true on success.

	\note All cells except the top-left one will be cleared.
 */
bool Worksheet::mergeCells(const CellRange &range, const Format &format)
{
	Q_D(Worksheet);
	if (range.rowCount() < 2 && range.columnCount() < 2)
		return false;

	if (d->checkDimensions(range.firstRow(), range.firstColumn()))
		return false;

	if (format.isValid())
		d->workbook->styles()->addXfFormat(format);

	for (int row = range.firstRow(); row <= range.lastRow(); ++row) {
		for (int col = range.firstColumn(); col <= range.lastColumn(); ++col) {
			if (row == range.firstRow() && col == range.firstColumn()) {
				Cell *cell = cellAt(row, col);
				if (cell) {
					if (format.isValid())
						cell->d_ptr->format = format;
				} else {
					writeBlank(row, col, format);
				}
			} else {
				writeBlank(row, col, format);
			}
		}
	}

	d->merges.append(range);
	return true;
}

/*!
	Unmerge the cells in the \a range. Returns true on success.

*/
bool Worksheet::unmergeCells(const CellRange &range)
{
	Q_D(Worksheet);
	if (!d->merges.contains(range))
		return false;

	d->merges.removeOne(range);
	return true;
}

/*!
  Returns all the merged cells.
*/
QList<CellRange> Worksheet::mergedCells() const
{
	Q_D(const Worksheet);
	return d->merges;
}

/*!
 * \internal
 */
void Worksheet::saveToXmlFile(QIODevice *device) const
{
	Q_D(const Worksheet);
	d->relationships->clear();

	QXmlStreamWriter writer(device);

	writer.writeStartDocument(QStringLiteral("1.0"), true);
	writer.writeStartElement(QStringLiteral("worksheet"));
	writer.writeAttribute(QStringLiteral("xmlns"), QStringLiteral("http://schemas.openxmlformats.org/spreadsheetml/2006/main"));
	writer.writeAttribute(QStringLiteral("xmlns:r"), QStringLiteral("http://schemas.openxmlformats.org/officeDocument/2006/relationships"));

	//for Excel 2010
	//writer.writeAttribute("xmlns:mc", "http://schemas.openxmlformats.org/markup-compatibility/2006");
	//writer.writeAttribute("xmlns:x14ac", "http://schemas.microsoft.com/office/spreadsheetml/2009/9/ac");
	//writer.writeAttribute("mc:Ignorable", "x14ac");

	//for Excel 365
	//writer.writeAttribute(QStringLiteral("mc:Ignorable"), QStringLiteral("x14ac xr xr2 xr3"));
	//writer.writeAttribute(QStringLiteral("xmlns:x14ac"), QStringLiteral("http://schemas.microsoft.com/office/spreadsheetml/2009/9/ac"));
	//writer.writeAttribute(QStringLiteral("xmlns:xr"), QStringLiteral("http://schemas.microsoft.com/office/spreadsheetml/2014/revision"));
	//writer.writeAttribute(QStringLiteral("xmlns:xr2"), QStringLiteral("http://schemas.microsoft.com/office/spreadsheetml/2015/revision2"));
	//writer.writeAttribute(QStringLiteral("xmlns:xr3"), QStringLiteral("http://schemas.microsoft.com/office/spreadsheetml/2016/revision3"));
	//writer.writeAttribute(QStringLiteral("xr:uid"), QStringLiteral("{00000000-0000-0000-0000-000000000000}"));

	writer.writeStartElement(QStringLiteral("dimension"));
	writer.writeAttribute(QStringLiteral("ref"), d->generateDimensionString());
	writer.writeEndElement();//dimension

	writer.writeStartElement(QStringLiteral("sheetViews"));
	writer.writeStartElement(QStringLiteral("sheetView"));
	if (d->windowProtection)
		writer.writeAttribute(QStringLiteral("windowProtection"), QStringLiteral("1"));
	if (d->showFormulas)
		writer.writeAttribute(QStringLiteral("showFormulas"), QStringLiteral("1"));
	if (!d->showGridLines)
		writer.writeAttribute(QStringLiteral("showGridLines"), QStringLiteral("0"));
	if (!d->showRowColHeaders)
		writer.writeAttribute(QStringLiteral("showRowColHeaders"), QStringLiteral("0"));
	if (!d->showZeros)
		writer.writeAttribute(QStringLiteral("showZeros"), QStringLiteral("0"));
	if (d->rightToLeft)
		writer.writeAttribute(QStringLiteral("rightToLeft"), QStringLiteral("1"));
	if (d->tabSelected)
		writer.writeAttribute(QStringLiteral("tabSelected"), QStringLiteral("1"));
	if (!d->showRuler)
		writer.writeAttribute(QStringLiteral("showRuler"), QStringLiteral("0"));
	if (!d->showOutlineSymbols)
		writer.writeAttribute(QStringLiteral("showOutlineSymbols"), QStringLiteral("0"));
	if (!d->showWhiteSpace)
		writer.writeAttribute(QStringLiteral("showWhiteSpace"), QStringLiteral("0"));
	writer.writeAttribute(QStringLiteral("workbookViewId"), QStringLiteral("0"));
	writer.writeEndElement();//sheetView
	writer.writeEndElement();//sheetViews

	writer.writeStartElement(QStringLiteral("sheetFormatPr"));
	writer.writeAttribute(QStringLiteral("defaultRowHeight"), QString::number(d->default_row_height));
	if (d->default_row_height != 15)
		writer.writeAttribute(QStringLiteral("customHeight"), QStringLiteral("1"));
	if (d->default_row_zeroed)
		writer.writeAttribute(QStringLiteral("zeroHeight"), QStringLiteral("1"));
	if (d->outline_row_level)
		writer.writeAttribute(QStringLiteral("outlineLevelRow"), QString::number(d->outline_row_level));
	if (d->outline_col_level)
		writer.writeAttribute(QStringLiteral("outlineLevelCol"), QString::number(d->outline_col_level));
	//for Excel 2010 and Excel 365
	//    writer.writeAttribute("x14ac:dyDescent", "0.25");
	writer.writeEndElement();//sheetFormatPr

	if (!d->colsInfo.isEmpty()) {
		writer.writeStartElement(QStringLiteral("cols"));
		QMapIterator<int, QSharedPointer<XlsxColumnInfo> > it(d->colsInfo);
		while (it.hasNext()) {
			it.next();
			QSharedPointer<XlsxColumnInfo> col_info = it.value();
			writer.writeStartElement(QStringLiteral("col"));
			writer.writeAttribute(QStringLiteral("min"), QString::number(col_info->firstColumn));
			writer.writeAttribute(QStringLiteral("max"), QString::number(col_info->lastColumn));
			if (col_info->width)
				writer.writeAttribute(QStringLiteral("width"), QString::number(col_info->width, 'g', 15));
			if (!col_info->format.isEmpty())
				writer.writeAttribute(QStringLiteral("style"), QString::number(col_info->format.xfIndex()));
			if (col_info->hidden)
				writer.writeAttribute(QStringLiteral("hidden"), QStringLiteral("1"));
			if (col_info->width)
				writer.writeAttribute(QStringLiteral("customWidth"), QStringLiteral("1"));
			if (col_info->outlineLevel)
				writer.writeAttribute(QStringLiteral("outlineLevel"), QString::number(col_info->outlineLevel));
			if (col_info->collapsed)
				writer.writeAttribute(QStringLiteral("collapsed"), QStringLiteral("1"));
			writer.writeEndElement();//col
		}
		writer.writeEndElement();//cols
	}

	writer.writeStartElement(QStringLiteral("sheetData"));
	if (d->dimension.isValid())
		d->saveXmlSheetData(writer);
	writer.writeEndElement();//sheetData

	d->saveXmlMergeCells(writer);
	foreach (const ConditionalFormatting cf, d->conditionalFormattingList)
		cf.saveToXml(writer);
	d->saveXmlDataValidations(writer);

	//{{ liufeijin
    // write  pagesettings  add by liufeijin 20181028

	//TODO: Implement option to define page margins for XLSX files
	//NOTE: Empty pageMargins element caused problem when document is opened by MS Excel
	/*
	writer.writeEmptyElement(QStringLiteral("pageMargins"));
	if(!d->PMleft.isEmpty())
	{
		writer.writeAttribute(QStringLiteral("left"), d->PMleft);
	}
	if(!d->PMright.isEmpty())
	{
		writer.writeAttribute(QStringLiteral("right"), d->PMright);
	}
	if(!d->PMtop.isEmpty())
	{
		writer.writeAttribute(QStringLiteral("top"),d->PMtop);
	}
	if(!d->PMbotton.isEmpty())
	{
		writer.writeAttribute(QStringLiteral("bottom"), d->PMbotton);
	}
	if(!d->PMheader.isEmpty())
	{
		writer.writeAttribute(QStringLiteral("header"), d->PMheader);
	}
	if(!d->PMfooter.isEmpty())
	{
		writer.writeAttribute(QStringLiteral("footer"), d->PMfooter);
	}
	*/

	writer.writeEmptyElement(QStringLiteral("pageSetup"));
	if(!d->PverticalDpi.isEmpty())
	{
		writer.writeAttribute(QStringLiteral("verticalDpi"), d->PverticalDpi);
	}
	if(!d->PhorizontalDpi.isEmpty())
	{
		writer.writeAttribute(QStringLiteral("horizontalDpi"), d->PhorizontalDpi);
	}
	if(!d->PuseFirstPageNumber.isEmpty())
	{
		writer.writeAttribute(QStringLiteral("useFirstPageNumber"), d->PuseFirstPageNumber);
	}
	if(!d->PfirstPageNumber.isEmpty())
	{
		writer.writeAttribute(QStringLiteral("firstPageNumber"), d->PfirstPageNumber);
	}
	if(!d->Pscale.isEmpty())
	{
		writer.writeAttribute(QStringLiteral("scale"), d->Pscale);
	}
	if(!d->PpaperSize.isEmpty())
	{
		writer.writeAttribute(QStringLiteral("paperSize"), d->PpaperSize);
	}
	if(!d->Porientation.isEmpty())
	{
		writer.writeAttribute(QStringLiteral("orientation"), d->Porientation);
	}
	if(!d->Pcopies.isEmpty())
	{
		writer.writeAttribute(QStringLiteral("copies"), d->Pcopies);
	}
	/*
	if(!d->Prid.isEmpty())
	{
		writer.writeAttribute(QStringLiteral("r:id"), d->Prid);
	}
	*/

	if((!d->MoodFooter.isNull())||!(d->MoodFooter.isNull()))
	{
		writer.writeStartElement(QStringLiteral("headerFooter")); // headerFooter
		if (!d->ModdHeader.isNull())
		{
			writer.writeTextElement(QStringLiteral("oddHeader"), d->ModdHeader);
		}
		if(!d->MoodFooter.isNull())
		{
			writer.writeTextElement(QStringLiteral("oddFooter"), d->MoodFooter);
		}
		writer.writeEndElement();// headerFooter
	}
	//}}

	d->saveXmlHyperlinks(writer);
	d->saveXmlDrawings(writer);

	writer.writeEndElement();//worksheet
	writer.writeEndDocument();
}

//{{ liufeijin 
bool Worksheet::setStartPage(int spagen)
{
	Q_D(Worksheet);
	d->PfirstPageNumber=QString::number(spagen);
	return true;
}
//}}

void WorksheetPrivate::saveXmlSheetData(QXmlStreamWriter &writer) const
{
	calculateSpans();
	for (int row_num = dimension.firstRow(); row_num <= dimension.lastRow(); row_num++)
	{
		if (!(cellTable.contains(row_num) || comments.contains(row_num) || rowsInfo.contains(row_num)))
		{
			//Only process rows with cell data / comments / formatting
			continue;
		}

		int span_index = (row_num-1) / 16;
		QString span;
		if (row_spans.contains(span_index))
			span = row_spans[span_index];

		writer.writeStartElement(QStringLiteral("row"));
		writer.writeAttribute(QStringLiteral("r"), QString::number(row_num));

		if (!span.isEmpty())
			writer.writeAttribute(QStringLiteral("spans"), span);

		if (rowsInfo.contains(row_num))
		{
			QSharedPointer<XlsxRowInfo> rowInfo = rowsInfo[row_num];
			if (!rowInfo->format.isEmpty())
			{
				writer.writeAttribute(QStringLiteral("s"), QString::number(rowInfo->format.xfIndex()));
				writer.writeAttribute(QStringLiteral("customFormat"), QStringLiteral("1"));
			}
			//!Todo: support customHeight from info struct
			//!Todo: where does this magic number '15' come from?
			if (rowInfo->customHeight)
			{
				writer.writeAttribute(QStringLiteral("ht"), QString::number(rowInfo->height));
				writer.writeAttribute(QStringLiteral("customHeight"), QStringLiteral("1"));
			} else {
				writer.writeAttribute(QStringLiteral("customHeight"), QStringLiteral("0"));
			}

			if (rowInfo->hidden)
				writer.writeAttribute(QStringLiteral("hidden"), QStringLiteral("1"));
			if (rowInfo->outlineLevel > 0)
				writer.writeAttribute(QStringLiteral("outlineLevel"), QString::number(rowInfo->outlineLevel));
			if (rowInfo->collapsed)
				writer.writeAttribute(QStringLiteral("collapsed"), QStringLiteral("1"));
		}

		//Write cell data if row contains filled cells
		if (cellTable.contains(row_num))
		{
			for (int col_num = dimension.firstColumn(); col_num <= dimension.lastColumn(); col_num++)
			{
				if (cellTable[row_num].contains(col_num))
				{
					saveXmlCellData(writer, row_num, col_num, cellTable[row_num][col_num]);
				}
			}
		}
		writer.writeEndElement(); //row
	}
}

void WorksheetPrivate::saveXmlCellData(QXmlStreamWriter &writer, int row, int col, QSharedPointer<Cell> cell) const
{
	//This is the innermost loop so efficiency is important.
	QString cell_pos = CellReference(row, col).toString();

	writer.writeStartElement(QStringLiteral("c"));
	writer.writeAttribute(QStringLiteral("r"), cell_pos);

	//Style used by the cell, row or col
	if (!cell->format().isEmpty())
		writer.writeAttribute(QStringLiteral("s"), QString::number(cell->format().xfIndex()));
	else if (rowsInfo.contains(row) && !rowsInfo[row]->format.isEmpty())
		writer.writeAttribute(QStringLiteral("s"), QString::number(rowsInfo[row]->format.xfIndex()));
	else if (colsInfoHelper.contains(col) && !colsInfoHelper[col]->format.isEmpty())
		writer.writeAttribute(QStringLiteral("s"), QString::number(colsInfoHelper[col]->format.xfIndex()));

	if (cell->cellType() == Cell::SharedStringType)
	{
		int sst_idx;
		if (cell->isRichString())
			sst_idx = sharedStrings()->getSharedStringIndex(cell->d_ptr->richString);
		else
			sst_idx = sharedStrings()->getSharedStringIndex(cell->value().toString());

		writer.writeAttribute(QStringLiteral("t"), QStringLiteral("s"));
		writer.writeTextElement(QStringLiteral("v"), QString::number(sst_idx));
	}
	else if (cell->cellType() == Cell::InlineStringType)
	{
		writer.writeAttribute(QStringLiteral("t"), QStringLiteral("inlineStr"));
		writer.writeStartElement(QStringLiteral("is"));
		if (cell->isRichString())
		{
			//Rich text string
			RichString string = cell->d_ptr->richString;
			for (int i=0; i<string.fragmentCount(); ++i)
			{
				writer.writeStartElement(QStringLiteral("r"));
				if (string.fragmentFormat(i).hasFontData())
				{
					writer.writeStartElement(QStringLiteral("rPr"));
					//:Todo
					writer.writeEndElement();// rPr
				}
				writer.writeStartElement(QStringLiteral("t"));
				if (isSpaceReserveNeeded(string.fragmentText(i)))
					writer.writeAttribute(QStringLiteral("xml:space"), QStringLiteral("preserve"));
				writer.writeCharacters(string.fragmentText(i));
				writer.writeEndElement();// t
				writer.writeEndElement(); // r
			}
		}
		else
		{
			writer.writeStartElement(QStringLiteral("t"));
			QString string = cell->value().toString();
			if (isSpaceReserveNeeded(string))
				writer.writeAttribute(QStringLiteral("xml:space"), QStringLiteral("preserve"));
			writer.writeCharacters(string);
			writer.writeEndElement(); // t
		}
		writer.writeEndElement();//is
	}
	else if (cell->cellType() == Cell::NumberType)
	{
		if (cell->hasFormula())
			cell->formula().saveToXml(writer);
		if (cell->value().isValid())
		{//note that, invalid value means 'v' is blank
			double value = cell->value().toDouble();
			writer.writeTextElement(QStringLiteral("v"), QString::number(value, 'g', 15));
		}
	}
	else if (cell->cellType() == Cell::StringType)
	{
		writer.writeAttribute(QStringLiteral("t"), QStringLiteral("str"));
		if (cell->hasFormula())
			cell->formula().saveToXml(writer);
		writer.writeTextElement(QStringLiteral("v"), cell->value().toString());
	}
	else if (cell->cellType() == Cell::BooleanType)
	{
		writer.writeAttribute(QStringLiteral("t"), QStringLiteral("b"));
		writer.writeTextElement(QStringLiteral("v"), cell->value().toBool() ? QStringLiteral("1") : QStringLiteral("0"));
	}
	writer.writeEndElement(); //c
}

void WorksheetPrivate::saveXmlMergeCells(QXmlStreamWriter &writer) const
{
	if (merges.isEmpty())
		return;

	writer.writeStartElement(QStringLiteral("mergeCells"));
	writer.writeAttribute(QStringLiteral("count"), QString::number(merges.size()));

	foreach (CellRange range, merges)
	{
		writer.writeEmptyElement(QStringLiteral("mergeCell"));
		writer.writeAttribute(QStringLiteral("ref"), range.toString());
	}

	writer.writeEndElement(); //mergeCells
}

void WorksheetPrivate::saveXmlDataValidations(QXmlStreamWriter &writer) const
{
	if (dataValidationsList.isEmpty())
		return;

	writer.writeStartElement(QStringLiteral("dataValidations"));
	writer.writeAttribute(QStringLiteral("count"), QString::number(dataValidationsList.size()));

	foreach (DataValidation validation, dataValidationsList)
		validation.saveToXml(writer);

	writer.writeEndElement(); //dataValidations
}

void WorksheetPrivate::saveXmlHyperlinks(QXmlStreamWriter &writer) const
{
	if (urlTable.isEmpty())
		return;

	writer.writeStartElement(QStringLiteral("hyperlinks"));
	QMapIterator<int, QMap<int, QSharedPointer<XlsxHyperlinkData> > > it(urlTable);
	while (it.hasNext())
	{
		it.next();
		int row = it.key();
		QMapIterator <int, QSharedPointer<XlsxHyperlinkData> > it2(it.value());
		while (it2.hasNext())
		{
			it2.next();
			int col = it2.key();
			QSharedPointer<XlsxHyperlinkData> data = it2.value();
			QString ref = CellReference(row, col).toString();
			writer.writeEmptyElement(QStringLiteral("hyperlink"));
			writer.writeAttribute(QStringLiteral("ref"), ref);
			if (data->linkType == XlsxHyperlinkData::External)
			{
				//Update relationships
				relationships->addWorksheetRelationship(QStringLiteral("/hyperlink"), data->target, QStringLiteral("External"));

				writer.writeAttribute(QStringLiteral("r:id"), QStringLiteral("rId%1").arg(relationships->count()));
			}

			if (!data->location.isEmpty())
				writer.writeAttribute(QStringLiteral("location"), data->location);
			if (!data->display.isEmpty())
				writer.writeAttribute(QStringLiteral("display"), data->display);
			if (!data->tooltip.isEmpty())
				writer.writeAttribute(QStringLiteral("tooltip"), data->tooltip);
		}
	}

	writer.writeEndElement();//hyperlinks
}

void WorksheetPrivate::saveXmlDrawings(QXmlStreamWriter &writer) const
{
	if (!drawing)
		return;

	int idx = workbook->drawings().indexOf(drawing.data());
	relationships->addWorksheetRelationship(QStringLiteral("/drawing"), QStringLiteral("../drawings/drawing%1.xml").arg(idx+1));

	writer.writeEmptyElement(QStringLiteral("drawing"));
	writer.writeAttribute(QStringLiteral("r:id"), QStringLiteral("rId%1").arg(relationships->count()));
}

void WorksheetPrivate::splitColsInfo(int colFirst, int colLast)
{
	// Split current columnInfo, for example, if "A:H" has been set,
	// we are trying to set "B:D", there should be "A", "B:D", "E:H".
	// This will be more complex if we try to set "C:F" after "B:D".
	{
		QMapIterator<int, QSharedPointer<XlsxColumnInfo> > it(colsInfo);
		while (it.hasNext())
		{
			it.next();
			QSharedPointer<XlsxColumnInfo> info = it.value();
			if (colFirst > info->firstColumn && colFirst <= info->lastColumn) {
				//split the range,
				QSharedPointer<XlsxColumnInfo> info2(new XlsxColumnInfo(*info));
				info->lastColumn = colFirst - 1;
				info2->firstColumn = colFirst;
				colsInfo.insert(colFirst, info2);
				for (int c = info2->firstColumn; c <= info2->lastColumn; ++c)
					colsInfoHelper[c] = info2;

				break;
			}
		}
	}
	{
		QMapIterator<int, QSharedPointer<XlsxColumnInfo> > it(colsInfo);
		while (it.hasNext())
		{
			it.next();
			QSharedPointer<XlsxColumnInfo> info = it.value();
			if (colLast >= info->firstColumn && colLast < info->lastColumn) {
				QSharedPointer<XlsxColumnInfo> info2(new XlsxColumnInfo(*info));
				info->lastColumn = colLast;
				info2->firstColumn = colLast + 1;
				colsInfo.insert(colLast + 1, info2);
				for (int c = info2->firstColumn; c <= info2->lastColumn; ++c)
					colsInfoHelper[c] = info2;

				break;
			}
		}
	}
}

bool WorksheetPrivate::isColumnRangeValid(int colFirst, int colLast)
{
	bool ignore_row = true;
	bool ignore_col = false;

	if (colFirst > colLast)
		return false;

	if (checkDimensions(1, colLast, ignore_row, ignore_col))
		return false;
	if (checkDimensions(1, colFirst, ignore_row, ignore_col))
		return false;

	return true;
}

QList<int> WorksheetPrivate ::getColumnIndexes(int colFirst, int colLast)
{
	splitColsInfo(colFirst, colLast);

	QList<int> nodes;
	nodes.append(colFirst);
	for (int col = colFirst; col <= colLast; ++col)
	{
		if (colsInfo.contains(col))
		{
			if (nodes.last() != col)
				nodes.append(col);
			int nextCol = colsInfo[col]->lastColumn + 1;
			if (nextCol <= colLast)
				nodes.append(nextCol);
		}
	}

	return nodes;
}

/*!
  Sets width in characters of a \a range of columns to \a width.
  Returns true on success.
 */
bool Worksheet::setColumnWidth(const CellRange &range, double width)
{
	if (!range.isValid())
		return false;

	return setColumnWidth(range.firstColumn(), range.lastColumn(), width);
}

/*!
  Sets format property of a \a range of columns to \a format. Columns are 1-indexed.
  Returns true on success.
 */
bool Worksheet::setColumnFormat(const CellRange& range, const Format &format)
{
	if (!range.isValid())
		return false;

	return setColumnFormat(range.firstColumn(), range.lastColumn(), format);
}

/*!
  Sets hidden property of a \a range of columns to \a hidden. Columns are 1-indexed.
  Hidden columns are not visible.
  Returns true on success.
 */
bool Worksheet::setColumnHidden(const CellRange &range, bool hidden)
{
	if (!range.isValid())
		return false;

	return setColumnHidden(range.firstColumn(), range.lastColumn(), hidden);
}

/*!
  Sets width in characters for columns [\a colFirst, \a colLast] to \a width.
  Columns are 1-indexed.
  Returns true on success.
 */
bool Worksheet::setColumnWidth(int colFirst, int colLast, double width)
{
	Q_D(Worksheet);

	QList <QSharedPointer<XlsxColumnInfo> > columnInfoList = d->getColumnInfoList(colFirst, colLast);
	foreach(QSharedPointer<XlsxColumnInfo>  columnInfo, columnInfoList)
	   columnInfo->width = width;

	return (columnInfoList.count() > 0);
}

/*!
  Sets format property of a range of columns [\a colFirst, \a colLast] to \a format.
  Columns are 1-indexed.
  Returns true on success.
 */
bool Worksheet::setColumnFormat(int colFirst, int colLast, const Format &format)
{
	Q_D(Worksheet);

	QList <QSharedPointer<XlsxColumnInfo> > columnInfoList = d->getColumnInfoList(colFirst, colLast);
	foreach(QSharedPointer<XlsxColumnInfo>  columnInfo, columnInfoList)
	   columnInfo->format = format;

	if(columnInfoList.count() > 0)
	{
	   d->workbook->styles()->addXfFormat(format);
	   return true;
	}

	return false;
}

/*!
  Sets hidden property of a range of columns [\a colFirst, \a colLast] to \a hidden.
  Columns are 1-indexed. Returns true on success.
 */
bool Worksheet::setColumnHidden(int colFirst, int colLast, bool hidden)
{
	Q_D(Worksheet);

	QList <QSharedPointer<XlsxColumnInfo> > columnInfoList = d->getColumnInfoList(colFirst, colLast);
	foreach(QSharedPointer<XlsxColumnInfo>  columnInfo, columnInfoList)
	   columnInfo->hidden = hidden;

	return (columnInfoList.count() > 0);
}

/*!
  Returns width of the \a column in characters of the normal font. Columns are 1-indexed.
 */
double Worksheet::columnWidth(int column)
{
	Q_D(Worksheet);

	QList <QSharedPointer<XlsxColumnInfo> > columnInfoList = d->getColumnInfoList(column, column);
	if (columnInfoList.count() == 1)
	   return columnInfoList.at(0)->width ;

	return d->sheetFormatProps.defaultColWidth;
}

/*!
  Returns formatting of the \a column. Columns are 1-indexed.
 */
Format Worksheet::columnFormat(int column)
{
	Q_D(Worksheet);

	QList <QSharedPointer<XlsxColumnInfo> > columnInfoList = d->getColumnInfoList(column, column);
	if (columnInfoList.count() == 1)
	   return columnInfoList.at(0)->format;

	return Format();
}

/*!
  Returns true if \a column is hidden. Columns are 1-indexed.
 */
bool Worksheet::isColumnHidden(int column)
{
	Q_D(Worksheet);

	QList <QSharedPointer<XlsxColumnInfo> > columnInfoList = d->getColumnInfoList(column, column);
	if (columnInfoList.count() == 1)
	   return columnInfoList.at(0)->hidden;

	return false;
}

/*!
  Sets the \a height of the rows including and between \a rowFirst and \a rowLast.
  Row height measured in point size.
  Rows are 1-indexed.

  Returns true if success.
*/
bool Worksheet::setRowHeight(int rowFirst,int rowLast, double height)
{
	Q_D(Worksheet);

	QList <QSharedPointer<XlsxRowInfo> > rowInfoList = d->getRowInfoList(rowFirst,rowLast);

	foreach(QSharedPointer<XlsxRowInfo> rowInfo, rowInfoList)
	{
		rowInfo->height = height;
		rowInfo->customHeight = true;
	}

	return rowInfoList.count() > 0;
}

/*!
  Sets the \a format of the rows including and between \a rowFirst and \a rowLast.
  Rows are 1-indexed.

  Returns true if success.
*/
bool Worksheet::setRowFormat(int rowFirst,int rowLast, const Format &format)
{
	Q_D(Worksheet);

	QList <QSharedPointer<XlsxRowInfo> > rowInfoList = d->getRowInfoList(rowFirst,rowLast);

	foreach(QSharedPointer<XlsxRowInfo> rowInfo, rowInfoList)
		rowInfo->format = format;

	d->workbook->styles()->addXfFormat(format);
	return rowInfoList.count() > 0;
}

/*!
  Sets the \a hidden proeprty of the rows including and between \a rowFirst and \a rowLast.
  Rows are 1-indexed. If hidden is true rows will not be visible.

  Returns true if success.
*/
bool Worksheet::setRowHidden(int rowFirst,int rowLast, bool hidden)
{
	Q_D(Worksheet);

	QList <QSharedPointer<XlsxRowInfo> > rowInfoList = d->getRowInfoList(rowFirst,rowLast);
	foreach(QSharedPointer<XlsxRowInfo> rowInfo, rowInfoList)
		rowInfo->hidden = hidden;

	return rowInfoList.count() > 0;
}

/*!
 Returns height of \a row in points.
*/
double Worksheet::rowHeight(int row)
{
	Q_D(Worksheet);
	int min_col = d->dimension.isValid() ? d->dimension.firstColumn() : 1;

	if (d->checkDimensions(row, min_col, false, true) || !d->rowsInfo.contains(row))
		return d->sheetFormatProps.defaultRowHeight; //return default on invalid row


	return d->rowsInfo[row]->height;
}

/*!
 Returns format of \a row.
*/
Format Worksheet::rowFormat(int row)
{
	Q_D(Worksheet);
	int min_col = d->dimension.isValid() ? d->dimension.firstColumn() : 1;
	if (d->checkDimensions(row, min_col, false, true) || !d->rowsInfo.contains(row))
		return Format(); //return default on invalid row

	return d->rowsInfo[row]->format;
}

/*!
 Returns true if \a row is hidden.
*/
bool Worksheet::isRowHidden(int row)
{
	Q_D(Worksheet);
	int min_col = d->dimension.isValid() ? d->dimension.firstColumn() : 1;
	if (d->checkDimensions(row, min_col, false, true) || !d->rowsInfo.contains(row))
		return false; //return default on invalid row

	return d->rowsInfo[row]->hidden;
}

/*!
   Groups rows from \a rowFirst to \a rowLast with the given \a collapsed.

   Returns false if error occurs.
 */
bool Worksheet::groupRows(int rowFirst, int rowLast, bool collapsed)
{
	Q_D(Worksheet);

	for (int row=rowFirst; row<=rowLast; ++row) {
		if (d->rowsInfo.contains(row)) {
			d->rowsInfo[row]->outlineLevel += 1;
		} else {
			QSharedPointer<XlsxRowInfo> info(new XlsxRowInfo);
			info->outlineLevel += 1;
			d->rowsInfo.insert(row, info);
		}
		if (collapsed)
			d->rowsInfo[row]->hidden = true;
	}
	if (collapsed) {
		if (!d->rowsInfo.contains(rowLast+1))
			d->rowsInfo.insert(rowLast+1, QSharedPointer<XlsxRowInfo>(new XlsxRowInfo));
		d->rowsInfo[rowLast+1]->collapsed = true;
	}
	return true;
}

/*!
	\overload

	Groups columns with the given \a range and \a collapsed.
 */
bool Worksheet::groupColumns(const CellRange &range, bool collapsed)
{
	if (!range.isValid())
		return false;

	return groupColumns(range.firstColumn(), range.lastColumn(), collapsed);
}

/*!
   Groups columns from \a colFirst to \a colLast with the given \a collapsed.
   Returns false if error occurs.
*/
bool Worksheet::groupColumns(int colFirst, int colLast, bool collapsed)
{
	Q_D(Worksheet);

	d->splitColsInfo(colFirst, colLast);

	QList<int> nodes;
	nodes.append(colFirst);
	for (int col = colFirst; col <= colLast; ++col) {
		if (d->colsInfo.contains(col)) {
			if (nodes.last() != col)
				nodes.append(col);
			int nextCol = d->colsInfo[col]->lastColumn + 1;
			if (nextCol <= colLast)
				nodes.append(nextCol);
		}
	}

	for (int idx = 0; idx < nodes.size(); ++idx) {
		int colStart = nodes[idx];
		if (d->colsInfo.contains(colStart)) {
			QSharedPointer<XlsxColumnInfo> info = d->colsInfo[colStart];
			info->outlineLevel += 1;
			if (collapsed)
				info->hidden = true;
		} else {
			int colEnd = (idx == nodes.size() - 1) ? colLast : nodes[idx+1] - 1;
			QSharedPointer<XlsxColumnInfo> info(new XlsxColumnInfo(colStart, colEnd));
			info->outlineLevel += 1;
			d->colsInfo.insert(colFirst, info);
			if (collapsed)
				info->hidden = true;
			for (int c = colStart; c <= colEnd; ++c)
				d->colsInfoHelper[c] = info;
		}
	}

	if (collapsed) {
		int col = colLast+1;
		d->splitColsInfo(col, col);
		if (d->colsInfo.contains(col))
			d->colsInfo[col]->collapsed = true;
		else {
			QSharedPointer<XlsxColumnInfo> info(new XlsxColumnInfo(col, col));
			info->collapsed = true;
			d->colsInfo.insert(col, info);
			d->colsInfoHelper[col] = info;
		}
	}

	return false;
}

/*!
	Return the range that contains cell data.
 */
CellRange Worksheet::dimension() const
{
	Q_D(const Worksheet);
	return d->dimension;
}

/*
 Convert the height of a cell from user's units to pixels. If the
 height hasn't been set by the user we use the default value. If
 the row is hidden it has a value of zero.
*/
int WorksheetPrivate::rowPixelsSize(int row) const
{
	double height;
	if (row_sizes.contains(row))
		height = row_sizes[row];
	else
		height = default_row_height;
	return static_cast<int>(4.0 / 3.0 *height);
}

/*
 Convert the width of a cell from user's units to pixels. Excel rounds
 the column width to the nearest pixel. If the width hasn't been set
 by the user we use the default value. If the column is hidden it
 has a value of zero.
*/
int WorksheetPrivate::colPixelsSize(int col) const
{
	double max_digit_width = 7.0; //For Calabri 11
	double padding = 5.0;
	int pixels = 0;

	if (col_sizes.contains(col)) {
		double width = col_sizes[col];
		if (width < 1)
			pixels = static_cast<int>(width * (max_digit_width + padding) + 0.5);
		else
			pixels = static_cast<int>(width * max_digit_width + 0.5) + padding;
	} else {
		pixels = 64;
	}
	return pixels;
}

void WorksheetPrivate::loadXmlSheetData(QXmlStreamReader &reader)
{
	Q_Q(Worksheet);
	Q_ASSERT(reader.name() == QLatin1String("sheetData"));

	while (!reader.atEnd() && !(reader.name() == QLatin1String("sheetData") && reader.tokenType() == QXmlStreamReader::EndElement)) 
	{
		if (reader.readNextStartElement()) 
		{
			if (reader.name() == QLatin1String("row")) 
			{
				QXmlStreamAttributes attributes = reader.attributes();

				if (attributes.hasAttribute(QLatin1String("customFormat"))
						|| attributes.hasAttribute(QLatin1String("customHeight"))
						|| attributes.hasAttribute(QLatin1String("hidden"))
						|| attributes.hasAttribute(QLatin1String("outlineLevel"))
						|| attributes.hasAttribute(QLatin1String("collapsed"))) 
				{
					QSharedPointer<XlsxRowInfo> info(new XlsxRowInfo);
					if (attributes.hasAttribute(QLatin1String("customFormat")) && attributes.hasAttribute(QLatin1String("s"))) {
						int idx = attributes.value(QLatin1String("s")).toString().toInt();
						info->format = workbook->styles()->xfFormat(idx);
					}

					if (attributes.hasAttribute(QLatin1String("customHeight"))) {
						info->customHeight = attributes.value(QLatin1String("customHeight")) == QLatin1String("1");
						//Row height is only specified when customHeight is set
						if(attributes.hasAttribute(QLatin1String("ht"))) {
							info->height = attributes.value(QLatin1String("ht")).toString().toDouble();
						}
					}

					//both "hidden" and "collapsed" default are false
					info->hidden = attributes.value(QLatin1String("hidden")) == QLatin1String("1");
					info->collapsed = attributes.value(QLatin1String("collapsed")) == QLatin1String("1");

					if (attributes.hasAttribute(QLatin1String("outlineLevel")))
						info->outlineLevel = attributes.value(QLatin1String("outlineLevel")).toString().toInt();

					//"r" is optional too.
					if (attributes.hasAttribute(QLatin1String("r"))) {
						int row = attributes.value(QLatin1String("r")).toString().toInt();
						rowsInfo[row] = info;
					}
				}
			} 
			else if (reader.name() == QLatin1String("c")) // Cell
			{ 
				//Cell
				QXmlStreamAttributes attributes = reader.attributes();
				QString r = attributes.value(QLatin1String("r")).toString();
				CellReference pos(r);

				//get format
				Format format;
				qint32 styleIndex = -1;
				if (attributes.hasAttribute(QLatin1String("s"))) // Style (defined in the styles.xml file)
				{ 
					//"s" == style index
					int idx = attributes.value(QLatin1String("s")).toString().toInt();
					format = workbook->styles()->xfFormat(idx);
					styleIndex = idx;
				}

				Cell::CellType cellType = Cell::NumberType;
				if (attributes.hasAttribute(QLatin1String("t"))) // Type 
				{
					QString typeString = attributes.value(QLatin1String("t")).toString();
					if (typeString == QLatin1String("s"))
					{
						cellType = Cell::SharedStringType; 
					}
					else if (typeString == QLatin1String("inlineStr"))
					{
						cellType = Cell::InlineStringType;
					}
					else if (typeString == QLatin1String("str"))
					{
						cellType = Cell::StringType;
					}
					else if (typeString == QLatin1String("b"))
					{
						cellType = Cell::BooleanType;
					}
					else if (typeString == QLatin1String("e"))
					{
						cellType = Cell::ErrorType;
					}
					else
					{
						cellType = Cell::NumberType;
					}
				}

				// create a heap of new cell
				QSharedPointer<Cell> cell(new Cell(QVariant(), cellType, format, q, styleIndex));

				while (!reader.atEnd() && !(reader.name() == QLatin1String("c") && reader.tokenType() == QXmlStreamReader::EndElement)) 
				{
					if (reader.readNextStartElement())
					{
						if (reader.name() == QLatin1String("f")) 
						{
							CellFormula &formula = cell->d_func()->formula;
							formula.loadFromXml(reader);
							if (formula.formulaType() == CellFormula::SharedType && !formula.formulaText().isEmpty()) 
							{
								sharedFormulaMap[formula.sharedIndex()] = formula;
							}
						} 
						else if (reader.name() == QLatin1String("v")) // Value 
						{
							// NOTICE: CHECK POINT 

							QString value = reader.readElementText();
							if (cellType == Cell::SharedStringType) 
							{
								int sst_idx = value.toInt();
								sharedStrings()->incRefByStringIndex(sst_idx);
								RichString rs = sharedStrings()->getSharedString(sst_idx);
								QString strPlainString = rs.toPlainString();
								cell->d_func()->value = strPlainString; 
								if (rs.isRichString())
									cell->d_func()->richString = rs;
							} 
							else if (cellType == Cell::NumberType) 
							{
								cell->d_func()->value = value.toDouble();
							} 
							else if (cellType == Cell::BooleanType) 
							{
								cell->d_func()->value = value.toInt() ? true : false;
							} 
							else 
							{ //Cell::ErrorType and Cell::StringType
								cell->d_func()->value = value;
							} 
						} else if (reader.name() == QLatin1String("is")) {
							while (!reader.atEnd() && !(reader.name() == QLatin1String("is") && reader.tokenType() == QXmlStreamReader::EndElement)) {
								if (reader.readNextStartElement()) {
									//:Todo, add rich text read support
									if (reader.name() == QLatin1String("t")) {
										cell->d_func()->value = reader.readElementText();
									}
								}
							}
						} else if (reader.name() == QLatin1String("extLst")) {
							//skip extLst element
							while (!reader.atEnd() && !(reader.name() == QLatin1String("extLst")
														&& reader.tokenType() == QXmlStreamReader::EndElement)) {
								reader.readNextStartElement();
							}
						}
					}
				}

				cellTable[pos.row()][pos.column()] = cell;
			}
		}
	}
}

void WorksheetPrivate::loadXmlColumnsInfo(QXmlStreamReader &reader)
{
	Q_ASSERT(reader.name() == QLatin1String("cols"));

	while (!reader.atEnd() && !(reader.name() == QLatin1String("cols") && reader.tokenType() == QXmlStreamReader::EndElement)) {
		reader.readNextStartElement();
		if (reader.tokenType() == QXmlStreamReader::StartElement) {
			if (reader.name() == QLatin1String("col")) {
				QSharedPointer<XlsxColumnInfo> info(new XlsxColumnInfo);

				QXmlStreamAttributes colAttrs = reader.attributes();
				int min = colAttrs.value(QLatin1String("min")).toString().toInt();
				int max = colAttrs.value(QLatin1String("max")).toString().toInt();
				info->firstColumn = min;
				info->lastColumn = max;

				//Flag indicating that the column width for the affected column(s) is different from the
				// default or has been manually set
				if(colAttrs.hasAttribute(QLatin1String("customWidth"))) {
					info->customWidth = colAttrs.value(QLatin1String("customWidth")) == QLatin1String("1");
				}
				//Note, node may have "width" without "customWidth"
				if (colAttrs.hasAttribute(QLatin1String("width"))) {
					double width = colAttrs.value(QLatin1String("width")).toString().toDouble();
					info->width = width;
				}

				info->hidden = colAttrs.value(QLatin1String("hidden")) == QLatin1String("1");
				info->collapsed = colAttrs.value(QLatin1String("collapsed")) == QLatin1String("1");

				if (colAttrs.hasAttribute(QLatin1String("style"))) {
					int idx = colAttrs.value(QLatin1String("style")).toString().toInt();
					info->format = workbook->styles()->xfFormat(idx);
				}
				if (colAttrs.hasAttribute(QLatin1String("outlineLevel")))
					info->outlineLevel = colAttrs.value(QLatin1String("outlineLevel")).toString().toInt();

				colsInfo.insert(min, info);
				for (int col=min; col<=max; ++col)
					colsInfoHelper[col] = info;
			}
		}
	}
}

void WorksheetPrivate::loadXmlMergeCells(QXmlStreamReader &reader)
{
	Q_ASSERT(reader.name() == QLatin1String("mergeCells"));

	QXmlStreamAttributes attributes = reader.attributes();
	int count = attributes.value(QLatin1String("count")).toString().toInt();

	while (!reader.atEnd() && !(reader.name() == QLatin1String("mergeCells") && reader.tokenType() == QXmlStreamReader::EndElement)) {
		reader.readNextStartElement();
		if (reader.tokenType() == QXmlStreamReader::StartElement) {
			if (reader.name() == QLatin1String("mergeCell")) {
				QXmlStreamAttributes attrs = reader.attributes();
				QString rangeStr = attrs.value(QLatin1String("ref")).toString();
				merges.append(CellRange(rangeStr));
			}
		}
	}

	if (merges.size() != count)
		qDebug("read merge cells error");
}

void WorksheetPrivate::loadXmlDataValidations(QXmlStreamReader &reader)
{
	Q_ASSERT(reader.name() == QLatin1String("dataValidations"));
	QXmlStreamAttributes attributes = reader.attributes();
	int count = attributes.value(QLatin1String("count")).toString().toInt();

	while (!reader.atEnd() && !(reader.name() == QLatin1String("dataValidations")
			&& reader.tokenType() == QXmlStreamReader::EndElement)) {
		reader.readNextStartElement();
		if (reader.tokenType() == QXmlStreamReader::StartElement
				&& reader.name() == QLatin1String("dataValidation")) {
			dataValidationsList.append(DataValidation::loadFromXml(reader));
		}
	}

	if (dataValidationsList.size() != count)
		qDebug("read data validation error");
}

void WorksheetPrivate::loadXmlSheetViews(QXmlStreamReader &reader)
{
	Q_ASSERT(reader.name() == QLatin1String("sheetViews"));

	while (!reader.atEnd() && !(reader.name() == QLatin1String("sheetViews")
			&& reader.tokenType() == QXmlStreamReader::EndElement)) {
		reader.readNextStartElement();
		if (reader.tokenType() == QXmlStreamReader::StartElement && reader.name() == QLatin1String("sheetView")) {
			QXmlStreamAttributes attrs = reader.attributes();
			//default false
			windowProtection = attrs.value(QLatin1String("windowProtection")) == QLatin1String("1");
			showFormulas = attrs.value(QLatin1String("showFormulas")) == QLatin1String("1");
			rightToLeft = attrs.value(QLatin1String("rightToLeft")) == QLatin1String("1");
			tabSelected = attrs.value(QLatin1String("tabSelected")) == QLatin1String("1");
			//default true
			showGridLines = attrs.value(QLatin1String("showGridLines")) != QLatin1String("0");
			showRowColHeaders = attrs.value(QLatin1String("showRowColHeaders")) != QLatin1String("0");
			showZeros = attrs.value(QLatin1String("showZeros")) != QLatin1String("0");
			showRuler = attrs.value(QLatin1String("showRuler")) != QLatin1String("0");
			showOutlineSymbols = attrs.value(QLatin1String("showOutlineSymbols")) != QLatin1String("0");
			showWhiteSpace = attrs.value(QLatin1String("showWhiteSpace")) != QLatin1String("0");
		}
	}
}

void WorksheetPrivate::loadXmlSheetFormatProps(QXmlStreamReader &reader)
{
	Q_ASSERT(reader.name() == QLatin1String("sheetFormatPr"));
	QXmlStreamAttributes attributes = reader.attributes();
	XlsxSheetFormatProps formatProps;

	//Retain default values
	foreach (QXmlStreamAttribute attrib, attributes) {
		if(attrib.name() == QLatin1String("baseColWidth") ) {
			formatProps.baseColWidth = attrib.value().toString().toInt();
		} else if(attrib.name() == QLatin1String("customHeight")) {
			formatProps.customHeight = attrib.value() == QLatin1String("1");
		} else if(attrib.name() == QLatin1String("defaultColWidth")) {
			formatProps.defaultColWidth = attrib.value().toString().toDouble();
		} else if(attrib.name() == QLatin1String("defaultRowHeight")) {
			formatProps.defaultRowHeight = attrib.value().toString().toDouble();
		} else if(attrib.name() == QLatin1String("outlineLevelCol")) {
			formatProps.outlineLevelCol = attrib.value().toString().toInt();
		} else if(attrib.name() == QLatin1String("outlineLevelRow")) {
			formatProps.outlineLevelRow = attrib.value().toString().toInt();
		} else if(attrib.name() == QLatin1String("thickBottom")) {
			formatProps.thickBottom = attrib.value() == QLatin1String("1");
		} else if(attrib.name() == QLatin1String("thickTop")) {
			formatProps.thickTop  = attrib.value() == QLatin1String("1");
		} else if(attrib.name() == QLatin1String("zeroHeight")) {
			formatProps.zeroHeight = attrib.value() == QLatin1String("1");
		}
	}

	if(formatProps.defaultColWidth == 0.0) { //not set
	   formatProps.defaultColWidth = WorksheetPrivate::calculateColWidth(formatProps.baseColWidth);
	}
}
double WorksheetPrivate::calculateColWidth(int characters)
{
	//!Todo
	//Take normal style' font maximum width and add padding and margin pixels
	return characters + 0.5;
}

void WorksheetPrivate::loadXmlHyperlinks(QXmlStreamReader &reader)
{
	Q_ASSERT(reader.name() == QLatin1String("hyperlinks"));

	while (!reader.atEnd() && !(reader.name() == QLatin1String("hyperlinks")
			&& reader.tokenType() == QXmlStreamReader::EndElement)) {
		reader.readNextStartElement();
		if (reader.tokenType() == QXmlStreamReader::StartElement && reader.name() == QLatin1String("hyperlink")) {
			QXmlStreamAttributes attrs = reader.attributes();
			CellReference pos(attrs.value(QLatin1String("ref")).toString());
			if (pos.isValid()) { //Valid
				QSharedPointer<XlsxHyperlinkData> link(new XlsxHyperlinkData);
				link->display = attrs.value(QLatin1String("display")).toString();
				link->tooltip = attrs.value(QLatin1String("tooltip")).toString();
				link->location = attrs.value(QLatin1String("location")).toString();

				if (attrs.hasAttribute(QLatin1String("r:id"))) {
					link->linkType = XlsxHyperlinkData::External;
					XlsxRelationship ship = relationships->getRelationshipById(attrs.value(QLatin1String("r:id")).toString());
					link->target = ship.target;
				} else {
					link->linkType = XlsxHyperlinkData::Internal;
				}

				urlTable[pos.row()][pos.column()] = link;
			}
		}
	}
}

QList <QSharedPointer<XlsxColumnInfo> > WorksheetPrivate::getColumnInfoList(int colFirst, int colLast)
{
	QList <QSharedPointer<XlsxColumnInfo> > columnsInfoList;
	if(isColumnRangeValid(colFirst,colLast))
	{
		QList<int> nodes = getColumnIndexes(colFirst, colLast);

		for (int idx = 0; idx < nodes.size(); ++idx) {
			int colStart = nodes[idx];
			if (colsInfo.contains(colStart)) {
				QSharedPointer<XlsxColumnInfo> info = colsInfo[colStart];
				columnsInfoList.append(info);
			} else {
				int colEnd = (idx == nodes.size() - 1) ? colLast : nodes[idx+1] - 1;
				QSharedPointer<XlsxColumnInfo> info(new XlsxColumnInfo(colStart, colEnd));
				colsInfo.insert(colFirst, info);
				columnsInfoList.append(info);
				for (int c = colStart; c <= colEnd; ++c)
					colsInfoHelper[c] = info;
			}
		}
	}

	return columnsInfoList;
}

QList <QSharedPointer<XlsxRowInfo> > WorksheetPrivate::getRowInfoList(int rowFirst, int rowLast)
{
	QList <QSharedPointer<XlsxRowInfo> > rowInfoList;

	int min_col = dimension.firstColumn() < 1 ? 1 : dimension.firstColumn();

	for(int row = rowFirst; row <= rowLast; ++row) {
		if (checkDimensions(row, min_col, false, true))
			continue;

		QSharedPointer<XlsxRowInfo> rowInfo;
		if ((rowsInfo[row]).isNull()){
			rowsInfo[row] = QSharedPointer<XlsxRowInfo>(new XlsxRowInfo());
		}
		rowInfoList.append(rowsInfo[row]);
	}

	return rowInfoList;
}

bool Worksheet::loadFromXmlFile(QIODevice *device)
{
	Q_D(Worksheet);

	QXmlStreamReader reader(device);
	while (!reader.atEnd()) {
		reader.readNextStartElement();
		if (reader.tokenType() == QXmlStreamReader::StartElement) {
			if (reader.name() == QLatin1String("dimension")) {
				QXmlStreamAttributes attributes = reader.attributes();
				QString range = attributes.value(QLatin1String("ref")).toString();
				d->dimension = CellRange(range);
			} else if (reader.name() == QLatin1String("sheetViews")) {
				d->loadXmlSheetViews(reader);
			} else if (reader.name() == QLatin1String("sheetFormatPr")) {
				d->loadXmlSheetFormatProps(reader);
			} else if (reader.name() == QLatin1String("cols")) {
				d->loadXmlColumnsInfo(reader);
			} else if (reader.name() == QLatin1String("sheetData")) {
				d->loadXmlSheetData(reader);
			} else if (reader.name() == QLatin1String("mergeCells")) {
				d->loadXmlMergeCells(reader);
			} else if (reader.name() == QLatin1String("dataValidations")) {
				d->loadXmlDataValidations(reader);
			} else if (reader.name() == QLatin1String("conditionalFormatting")) {
				ConditionalFormatting cf;
				cf.loadFromXml(reader, workbook()->styles());
				d->conditionalFormattingList.append(cf);
			} else if (reader.name() == QLatin1String("hyperlinks")) {
				d->loadXmlHyperlinks(reader);

//{{
} else if(reader.name() == QLatin1String("pageSetup")){
                QXmlStreamAttributes attributes = reader.attributes();
                d->PpaperSize = attributes.value(QLatin1String("paperSize")).toString().trimmed();
                d->Pscale = attributes.value(QLatin1String("scale")).toString().trimmed();
                d->PfirstPageNumber = attributes.value(QLatin1String("firstPageNumber")).toString().trimmed();
                d->Porientation = attributes.value(QLatin1String("orientation")).toString().trimmed();
                d->PuseFirstPageNumber = attributes.value(QLatin1String("useFirstPageNumber")).toString().trimmed();
                d->PhorizontalDpi = attributes.value(QLatin1String("horizontalDpi")).toString().trimmed();
                d->PverticalDpi = attributes.value(QLatin1String("verticalDpi")).toString().trimmed();
                d->Prid=attributes.value(QLatin1String("r:id")).toString().trimmed();
                d->Pcopies=attributes.value(QLatin1String("copies")).toString().trimmed();
            } else if(reader.name() == QLatin1String("pageMargins")){
                QXmlStreamAttributes attributes = reader.attributes();
                d->PMfooter= attributes.value(QLatin1String("footer")).toString().trimmed();
                d->PMheader = attributes.value(QLatin1String("header")).toString().trimmed();
                d->PMbotton = attributes.value(QLatin1String("bottom")).toString().trimmed();
                d->PMtop = attributes.value(QLatin1String("top")).toString().trimmed();
                d->PMright = attributes.value(QLatin1String("right")).toString().trimmed();
                d->PMleft = attributes.value(QLatin1String("left")).toString().trimmed();
            } else if(reader.name() == QLatin1String("headerFooter")){
                    reader.readNextStartElement();
                    if ((reader.tokenType() == QXmlStreamReader::StartElement)&&\
                             reader.name() == QLatin1String("oddHeader")) {
                        d->ModdHeader=reader.readElementText();
            }
//}}

			} else if (reader.name() == QLatin1String("drawing")) {
				QString rId = reader.attributes().value(QStringLiteral("r:id")).toString();
				QString name = d->relationships->getRelationshipById(rId).target;
				QString path = QDir::cleanPath(splitPath(filePath())[0] + QLatin1String("/") + name);
				d->drawing = QSharedPointer<Drawing>(new Drawing(this, F_LoadFromExists));
				d->drawing->setFilePath(path);
			} else if (reader.name() == QLatin1String("extLst")) {
				//Todo: add extLst support
				while (!reader.atEnd() && !(reader.name() == QLatin1String("extLst")
											&& reader.tokenType() == QXmlStreamReader::EndElement)) {
					reader.readNextStartElement();
				}
			}
		}
	}

	d->validateDimension();
	return true;
}

/*
 *  Documents imported from Google Docs does not contain dimension data.
 */
void WorksheetPrivate::validateDimension()
{
	if (dimension.isValid() || cellTable.isEmpty())
		return;

	int firstRow = cellTable.constBegin().key();
	int lastRow = (cellTable.constEnd()-1).key();
	int firstColumn = -1;
	int lastColumn = -1;

	for (QMap<int, QMap<int, QSharedPointer<Cell> > >::const_iterator it = cellTable.begin(); it != cellTable.end(); ++it)
	{
		Q_ASSERT(!it.value().isEmpty());

		if (firstColumn == -1 || it.value().constBegin().key() < firstColumn)
			firstColumn = it.value().constBegin().key();

		if (lastColumn == -1 || (it.value().constEnd()-1).key() > lastColumn)
			lastColumn = (it.value().constEnd()-1).key();
	}

	CellRange cr(firstRow, firstColumn, lastRow, lastColumn);

	if (cr.isValid())
		dimension = cr;
}

/*!
 * \internal
 *  Unit test can use this member to get sharedString object.
 */
SharedStrings *WorksheetPrivate::sharedStrings() const
{
	return workbook->sharedStrings();
}

QVector<CellLocation> Worksheet::getFullCells(int* maxRow, int* maxCol)
{
    Q_D(const Worksheet);

    (*maxRow) = -1;
    (*maxCol) = -1;
    QVector<CellLocation> ret;

    QMapIterator<int, QMap<int, QSharedPointer<Cell> > > _it(d->cellTable);
    while ( _it.hasNext() )
    {
        _it.next();

        int keyI = _it.key(); // key
        QMapIterator<int, QSharedPointer<Cell> > _iit( _it.value() ); // value
        while ( _iit.hasNext() )
        {
            _iit.next();

            int keyII = _iit.key(); // key
            QSharedPointer<Cell> ptrCell = _iit.value(); // value

            CellLocation cl;

            cl.row = keyI;
            if ( keyI > (*maxRow) )
                (*maxRow) = keyI;

            cl.col = keyII;
            if ( keyII > (*maxCol) )
                (*maxCol) = keyII;

            cl.cell = ptrCell;

            ret.push_back( cl );
        }
    }

    return ret;
}

QT_END_NAMESPACE_XLSX
