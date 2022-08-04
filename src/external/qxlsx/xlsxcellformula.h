// xlsxcellformula.h

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
    enum FormulaType { NormalType, ArrayType, DataTableType, SharedType };

public:
    CellFormula();
    CellFormula(const char *formula, FormulaType type=NormalType);
    CellFormula(const QString &formula, FormulaType type=NormalType);
    CellFormula(const QString &formula, const CellRange &ref, FormulaType type);
    CellFormula(const CellFormula &other);
    ~CellFormula();

public:
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
