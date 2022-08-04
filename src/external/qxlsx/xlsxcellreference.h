// xlsxcellreference.h

#ifndef QXLSX_XLSXCELLREFERENCE_H
#define QXLSX_XLSXCELLREFERENCE_H

#include <QtGlobal>

#include "xlsxglobal.h"

QT_BEGIN_NAMESPACE_XLSX

class   CellReference
{
public:
    CellReference();
    CellReference(int row, int column);
    CellReference(const QString &cell);
    CellReference(const char *cell);
    CellReference(const CellReference &other);
    ~CellReference();

    QString toString(bool row_abs=false, bool col_abs=false) const;
    static CellReference fromString(const QString &cell);
    bool isValid() const;
    inline void setRow(int row) { _row = row; }
    inline void setColumn(int col) { _column = col; }
    inline int row() const { return _row; }
    inline int column() const { return _column; }

    inline bool operator ==(const CellReference &other) const
    {
        return _row==other._row && _column==other._column;
    }
    inline bool operator !=(const CellReference &other) const
    {
        return _row!=other._row || _column!=other._column;
    }
private:
    void init(const QString &cell);
    int _row, _column;
};

QT_END_NAMESPACE_XLSX

Q_DECLARE_TYPEINFO(QXlsx::CellReference, Q_MOVABLE_TYPE);

#endif // QXLSX_XLSXCELLREFERENCE_H
