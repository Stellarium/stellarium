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
#ifndef QXLSX_XLSXCELLFORMULA_H
#define QXLSX_XLSXCELLFORMULA_H

#include "xlsxglobal.h"
#include <QExplicitlySharedDataPointer>

class QXmlStreamWriter;
class QXmlStreamReader;

QT_BEGIN_NAMESPACE_XLSX

class CellFormulaPrivate;
class CellRange;
class Worksheet;
class WorksheetPrivate;

class   CellFormula
{
public:
    enum FormulaType {
        NormalType,
        ArrayType,
        DataTableType,
        SharedType
    };

    CellFormula();
    CellFormula(const char *formula, FormulaType type=NormalType);
    CellFormula(const QString &formula, FormulaType type=NormalType);
    CellFormula(const QString &formula, const CellRange &ref, FormulaType type);
    CellFormula(const CellFormula &other);
    ~CellFormula();
    CellFormula &operator =(const CellFormula &other);
    bool isValid() const;

    FormulaType formulaType() const;
    QString formulaText() const;
    CellRange reference() const;
    int sharedIndex() const;

    bool operator == (const CellFormula &formula) const;
    bool operator != (const CellFormula &formula) const;

    bool saveToXml(QXmlStreamWriter &writer) const;
    bool loadFromXml(QXmlStreamReader &reader);
private:
    friend class Worksheet;
    friend class WorksheetPrivate;
    QExplicitlySharedDataPointer<CellFormulaPrivate> d;
};

QT_END_NAMESPACE_XLSX

#endif // QXLSX_XLSXCELLFORMULA_H
