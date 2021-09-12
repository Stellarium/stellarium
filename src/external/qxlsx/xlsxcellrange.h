// xlsxcellrange.h

#ifndef QXLSX_XLSXCELLRANGE_H
#define QXLSX_XLSXCELLRANGE_H

#include <QtGlobal>
#include <QObject>

#include "xlsxglobal.h"
#include "xlsxcellreference.h"

QT_BEGIN_NAMESPACE_XLSX

// dev57
class CellRange
{
public:
    CellRange();
    CellRange(int firstRow, int firstColumn, int lastRow, int lastColumn);
    CellRange(const CellReference &topLeft, const CellReference &bottomRight);
    CellRange(const QString &range);
    CellRange(const char *range);
    CellRange(const CellRange &other);
    ~CellRange();

    QString toString(bool row_abs=false, bool col_abs=false) const;
    bool isValid() const;
    inline void setFirstRow(int row) { top = row; }
    inline void setLastRow(int row) { bottom = row; }
    inline void setFirstColumn(int col) { left = col; }
    inline void setLastColumn(int col) { right = col; }
    inline int firstRow() const { return top; }
    inline int lastRow() const { return bottom; }
    inline int firstColumn() const { return left; }
    inline int lastColumn() const { return right; }
    inline int rowCount() const { return bottom - top + 1; }
    inline int columnCount() const { return right - left + 1; }
    inline CellReference topLeft() const { return CellReference(top, left); }
    inline CellReference topRight() const { return CellReference(top, right); }
    inline CellReference bottomLeft() const { return CellReference(bottom, left); }
    inline CellReference bottomRight() const { return CellReference(bottom, right); }

    inline void operator =(const CellRange &other)
    {
        top = other.top;
        bottom = other.bottom;
        left = other.left;
        right = other.right;
    }
    inline bool operator ==(const CellRange &other) const
    {
        return top==other.top && bottom==other.bottom
                && left == other.left && right == other.right;
    }
    inline bool operator !=(const CellRange &other) const
    {
        return top!=other.top || bottom!=other.bottom
                || left != other.left || right != other.right;
    }
private:
    void init(const QString &range);

    int top;
    int left;
    int bottom;
    int right;
};

QT_END_NAMESPACE_XLSX

Q_DECLARE_TYPEINFO(QXlsx::CellRange, Q_MOVABLE_TYPE);

#endif // QXLSX_XLSXCELLRANGE_H
