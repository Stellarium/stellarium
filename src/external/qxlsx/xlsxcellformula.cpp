// xlsxcellformula.cpp

#include <QtGlobal>
#include <QObject>
#include <QString>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QDebug>

#include "xlsxcellformula.h"
#include "xlsxcellformula_p.h"
#include "xlsxutility_p.h"

QT_BEGIN_NAMESPACE_XLSX

CellFormulaPrivate::CellFormulaPrivate(const QString &formula_, const CellRange &ref_, CellFormula::FormulaType type_)
    :formula(formula_), type(type_), reference(ref_), ca(false), si(0)
{
    //Remove the formula '=' sign if exists
    if (formula.startsWith(QLatin1String("=")))
        formula.remove(0,1);
    else if (formula.startsWith(QLatin1String("{=")) && formula.endsWith(QLatin1String("}")))
        formula = formula.mid(2, formula.length()-3);
}

CellFormulaPrivate::CellFormulaPrivate(const CellFormulaPrivate &other)
    : QSharedData(other)
    , formula(other.formula), type(other.type), reference(other.reference)
    , ca(other.ca), si(other.si)
{

}

CellFormulaPrivate::~CellFormulaPrivate()
{

}

/*!
  \class CellFormula
  \inmodule QtXlsx
  \brief The CellFormula class provides a API that is used to handle the cell formula.

*/

/*!
  \enum CellFormula::FormulaType
  \value NormalType
  \value ArrayType
  \value DataTableType
  \value SharedType
*/

/*!
 *  Creates a new formula.
 */
CellFormula::CellFormula()
{
    //The d pointer is initialized with a null pointer
}

/*!
 *  Creates a new formula with the given \a formula and \a type.
 */
CellFormula::CellFormula(const char *formula, FormulaType type)
    :d(new CellFormulaPrivate(QString::fromLatin1(formula), CellRange(), type))
{

}

/*!
 *  Creates a new formula with the given \a formula and \a type.
 */
CellFormula::CellFormula(const QString &formula, FormulaType type)
    :d(new CellFormulaPrivate(formula, CellRange(), type))
{

}

/*!
 *  Creates a new formula with the given \a formula, \a ref and \a type.
 */
CellFormula::CellFormula(const QString &formula, const CellRange &ref, FormulaType type)
    :d(new CellFormulaPrivate(formula, ref, type))
{

}

/*!
   Creates a new formula with the same attributes as the \a other formula.
 */
CellFormula::CellFormula(const CellFormula &other)
    :d(other.d)
{
}

/*!
   Assigns the \a other formula to this formula, and returns a
   reference to this formula.
 */
CellFormula &CellFormula::operator =(const CellFormula &other)
{
    d = other.d;
    return *this;
}

/*!
 * Destroys this formula.
 */
CellFormula::~CellFormula()
{

}

/*!
 * Returns the type of the formula.
 */
CellFormula::FormulaType CellFormula::formulaType() const
{
    return d ? d->type : NormalType;
}

/*!
 * Returns the contents of the formula.
 */
QString CellFormula::formulaText() const
{
    return d ? d->formula : QString();
}

/*!
 * Returns the reference cells of the formula. For normal formula,
 * this will return an invalid CellRange object.
 */
CellRange CellFormula::reference() const
{
    return d ? d->reference : CellRange();
}

/*!
 * Returns whether the formula is valid.
 */
bool CellFormula::isValid() const
{
    return d;
}

/*!
 * Returns the shared index for shared formula.
 */
int CellFormula::sharedIndex() const
{
    return d && d->type == SharedType ? d->si : (-1);
}

/* aca (Always Calculate Array) // not-implmented attribute
 *
 * Only applies to array formulas.
 *
 * true indicates that the entire array shall be calculated in full.
 * If false the individual cells of the array shall be calculated as needed.
 *
 * The aca value shall be ignored unless the value of the corresponding
 *  t attribute is array.
 *
 *  [Note: The primary case where an array formula must be calculated in
 * part instead of in full is when some cells in the array depend on other
 * cells that are semi-calculated, e.g., contains the function =(). end note]
 *
 *  The possible values for this attribute are defined by the W3C XML Schema
 *  boolean datatype.
 */

/* bx (Assigns Value to Name) // not-implmented attribute
 *
 * Specifies that this formula assigns a value to a name.
 *
 * The possible values for this attribute are defined by the W3C XML
 * Schema boolean datatype.
 */

/* del1 (Input 1 Deleted) // not-implmented attribute
 *
 * Whether the first input cell for data table has been deleted.
 * Applies to data table formula only. Written on master cell of data table
 * formula only.
 *
 * The possible values for this attribute are defined by the W3C XML Schema
 * boolean datatype.
*/

/* del2 (Input 2 Deleted) // not-impplmented attribute
 *
 * Whether the second input cell for data table has been deleted.
 * Applies to data table formula only. Written on master cell of data
 * table formula only.
 *
 * The possible values for this attribute are defined by the W3C XML Schema
 * boolean datatype.
 */

/* dt2D (Data Table 2-D) // not-implmented attribute
 *
 * Data table is two-dimentional. Only applies to the data tables function.
 * Written on master cell of data table formula only.
 *
 * The possible values for this attribute are defined by the W3C XML Schema
 * boolean datatype.
 */

/* dtr (Data Table Row) // not-implmented attribute
 *
 * true if one-dimentional data table is a row, otherwise it's a column.
 * Only applies to the data tables function. Written on master cell of data
 * table formula only.
 *
 * The possible values for this attribute are defined by the W3C XML Schema
 *  boolean datatype.
 */

/* r1 (Data Table Cell 1) // not-implmented attribute
 *
 * First input cell for data table. Only applies to the data tables array
 * function "TABLE()". Written on master cell of data table formula only.
 *
 * The possible values for this attribute are defined by the ST_CellRef
 * simple type (§18.18.7).
 */

/* r2 (Input Cell 2) // not-implmented attribute
 *
 * Second input cell for data table when dt2D is '1'. Only applies to the
 * data tables array function "TABLE()".Written on master cell of data table
 * formula only.
 *
 * The possible values for this attribute are defined by the ST_CellRef
 * simple type (§18.18.7).
 */

/*!
 * \internal
 * \remark pair with loadFromXml()
 */
bool CellFormula::saveToXml(QXmlStreamWriter &writer) const
{

    // t (Formula Type)
    //
    // Type of formula.
    // The possible values for this attribute are defined by the
    // ST_CellFormulaType simple type (§18.18.6).
    //
    // 18.18.6 ST_CellFormulaType (Formula Type)
    // array (Array Formula)
    // dataTable (Table Formula)
    // normal (Normal)
    // shared (Shared Formula)

    QString t;
    switch (d->type)
    {
    case CellFormula::ArrayType:
        t = QStringLiteral("array");
        break;
    case CellFormula::SharedType:
        t = QStringLiteral("shared");
        break;
    case CellFormula::NormalType:
        t = QStringLiteral("normal");
        break;
    case CellFormula::DataTableType:
        t = QStringLiteral("dataTable");
        break;
    default: // undefined type
        return false;
        break;
    }

    // f (Formula)
    //
    // Formula for the cell. The formula expression is contained in the
    // character node of this element.
    writer.writeStartElement(QStringLiteral("f"));

    if (!t.isEmpty())
    {
        writer.writeAttribute(QStringLiteral("t"), t); // write type(t)
    }

    // ref (Range of Cells)
    //
    // Range of cells which the formula applies to.
    // Only required for shared formula, array formula or data table.
    // Only written on the master formula,
    // not subsequent formulas belonging to the same shared group, array,
    // or data table.
    // The possible values for this attribute are defined by the ST_Ref
    // simple type (§18.18.62).

    if ( d->type == CellFormula::SharedType ||
         d->type == CellFormula::ArrayType ||
         d->type == CellFormula::DataTableType )
    {
        if (d->reference.isValid())
        {
            writer.writeAttribute(QStringLiteral("ref"), d->reference.toString());
        }
    }

    // ca (Calculate Cell)
    //
    // Indicates that this formula needs to be recalculated the next time
    // calculation is performed. [Example: This is always set on volatile
    // functions, like =(), and circular references. end example]
    // The possible values for this attribute are defined by the W3C XML
    // Schema boolean datatype.
    //
    // 3.2.2 boolean
    // 3.2.2.1 Lexical representation
    // An instance of a datatype that is defined as ·boolean· can have the
    // following legal literals {true, false, 1, 0}.

    if (d->ca)
    {
        writer.writeAttribute(QStringLiteral("ca"), QStringLiteral("1"));
    }

    // si (Shared Group Index)
    // Optional attribute to optimize load performance by sharing formulas.
    //
    // When a formula is a shared formula (t value is shared) then this value
    // indicates the group to which this particular cell's formula belongs. The
    // first formula in a group of shared formulas is saved in the f element.
    // This is considered the 'master' formula cell. Subsequent cells sharing
    // this formula need not have the formula written in their f element.
    // Instead, the attribute si value for a particular cell is used to figure
    // what the formula expression should be based on the cell's relative
    // location to the master formula cell.

    if (d->type == CellFormula::SharedType)
    {
        int si = d->si;
        writer.writeAttribute(QStringLiteral("si"), QString::number(si));
    }

    if (!d->formula.isEmpty())
    {
        QString strFormula = d->formula;
        writer.writeCharacters(strFormula); // write formula
    }

    writer.writeEndElement(); // f

    return true;
}

/*!
 * \internal
 * \remark pair with saveToXml()
 */
bool CellFormula::loadFromXml(QXmlStreamReader &reader)
{
    Q_ASSERT(reader.name() == QLatin1String("f"));
    if (!d)
        d = new CellFormulaPrivate(QString(), CellRange(), NormalType);

    QXmlStreamAttributes attributes = reader.attributes();
    QString typeString = attributes.value(QLatin1String("t")).toString();

    // branch: shared-formula
    //
    if (typeString == QLatin1String("array")) {
        d->type = ArrayType;
    }
    else if (typeString == QLatin1String("shared")) {
        d->type = SharedType;
    }
    else if (typeString == QLatin1String("normal")) {
        d->type = NormalType;
    }
    else if (typeString == QLatin1String("dataTable")) {
        d->type = DataTableType;
    }
    else {
        /*
        // undefined type
        // qDebug() << "Undefined type" << typeString;
        return false;
        // */

        // dev40 {{
        // https://github.com/QtExcel/QXlsx/issues/38
        d->type = NormalType; // Change: normal Type is not mentioned in the xml file!!!!!
        // }}
    }

    // branch: shared-formula
    //
    // ref (Range of Cells)
    // Range of cells which the formula applies to.
    // Only required for shared formula, array formula or data table.
    if ( d->type == CellFormula::SharedType ||
         d->type == CellFormula::ArrayType ||
         d->type == CellFormula::DataTableType )
    {
        if (attributes.hasAttribute(QLatin1String("ref")))
        {
            QString refString = attributes.value(QLatin1String("ref")).toString();
            d->reference = CellRange(refString);
        }
    }

    // branch: shared-formula
    //
    // si (Shared Group Index)
    // Optional attribute to optimize load performance by sharing formulas.
    // When a formula is a shared formula (t value is shared) then this value
    // indicates the group to which this particular cell's formula belongs.
    if ( d->type == CellFormula::SharedType )
    {
        QString ca = attributes.value(QLatin1String("si")).toString();
        d->ca = parseXsdBoolean(ca, false);

        if (attributes.hasAttribute(QLatin1String("si")))
        {
            d->si = attributes.value(QLatin1String("si")).toString().toInt();
        }
    }

    d->formula = reader.readElementText(); // read formula

    return true;
}

/*!
 * \internal
 */
bool CellFormula::operator ==(const CellFormula &formula) const
{
    return d->formula == formula.d->formula && d->type == formula.d->type
            && d->si ==formula.d->si;
}

/*!
 * \internal
 */
bool CellFormula::operator !=(const CellFormula &formula) const
{
    return d->formula != formula.d->formula || d->type != formula.d->type
            || d->si !=formula.d->si;
}

QT_END_NAMESPACE_XLSX
