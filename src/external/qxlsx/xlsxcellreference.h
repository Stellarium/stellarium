/****************************************************************************
** Copyright (c) 2013-2014 Debao Zhang <hello@debao.me>
** All right reserved.
**
** Permission is hereby granted, free of charge, to any person obtaining
** a copy of this software and associated documentation files (the
** "Software"), to deal in the Software without restriction, including
** without limitation the rights to use, copy, modify, merge, publish,
** distribute, sublicense, and/or sell copies of the Software, and to
** permit persons to whom the Software is furnished to do so, subject to
** the following conditions:
**
** The above copyright notice and this permission notice shall be
** included in all copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
** NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
** LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
** OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
** WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**
****************************************************************************/
#ifndef QXLSX_XLSXCELLREFERENCE_H
#define QXLSX_XLSXCELLREFERENCE_H
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
