// xlsxcellrange.cpp

#include <QtGlobal>
#include <QString>
#include <QPoint>
#include <QStringList>

#include "xlsxcellrange.h"
#include "xlsxcellreference.h"

QT_BEGIN_NAMESPACE_XLSX

/*!
    \class CellRange
    \brief For a range "A1:B2" or single cell "A1"
    \inmodule QtXlsx

    The CellRange class stores the top left and bottom
    right rows and columns of a range in a worksheet.
*/

/*!
    Constructs an range, i.e. a range
    whose rowCount() and columnCount() are 0.
*/
CellRange::CellRange()
    : top(-1), left(-1), bottom(-2), right(-2)
{
}

/*!
    Constructs the range from the given \a top, \a
    left, \a bottom and \a right rows and columns.

    \sa topRow(), leftColumn(), bottomRow(), rightColumn()
*/
CellRange::CellRange(int top, int left, int bottom, int right)
    : top(top), left(left), bottom(bottom), right(right)
{
}

CellRange::CellRange(const CellReference &topLeft, const CellReference &bottomRight)
    : top(topLeft.row()), left(topLeft.column())
    , bottom(bottomRight.row()), right(bottomRight.column())
{
}

/*!
    \overload
    Constructs the range form the given \a range string.
*/
CellRange::CellRange(const QString &range)
{
    init(range);
}

/*!
    \overload
    Constructs the range form the given \a range string.
*/
CellRange::CellRange(const char *range)
{
    init(QString::fromLatin1(range));
}

void CellRange::init(const QString &range)
{
    QStringList rs = range.split(QLatin1Char(':'));
    if (rs.size() == 2) {
        CellReference start(rs[0]);
        CellReference end(rs[1]);
        top = start.row();
        left = start.column();
        bottom = end.row();
        right = end.column();
    } else {
        CellReference p(rs[0]);
        top = p.row();
        left = p.column();
        bottom = p.row();
        right = p.column();
    }
}

/*!
    Constructs a the range by copying the given \a
    other range.
*/
CellRange::CellRange(const CellRange &other)
    : top(other.top), left(other.left), bottom(other.bottom), right(other.right)
{
}

/*!
    Destroys the range.
*/
CellRange::~CellRange()
{
}

/*!
     Convert the range to string notation, such as "A1:B5".
*/
QString CellRange::toString(bool row_abs, bool col_abs) const
{
    if (!isValid())
        return QString();

    if (left == right && top == bottom) {
        //Single cell
        return CellReference(top, left).toString(row_abs, col_abs);
    }

    QString cell_1 = CellReference(top, left).toString(row_abs, col_abs);
    QString cell_2 = CellReference(bottom, right).toString(row_abs, col_abs);
    return cell_1 + QLatin1String(":") + cell_2;
}

/*!
 * Returns true if the Range is valid.
 */
bool CellRange::isValid() const
{
    return left <= right && top <= bottom;
}

QT_END_NAMESPACE_XLSX
