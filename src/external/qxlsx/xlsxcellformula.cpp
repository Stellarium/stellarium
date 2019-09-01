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
#include "xlsxcellformula.h"
#include "xlsxcellformula_p.h"
#include "xlsxutility_p.h"

#include <QXmlStreamReader>
#include <QXmlStreamWriter>

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
    return d && d->type == SharedType ? d->si : -1;
}

/*!
 * \internal
 */
bool CellFormula::saveToXml(QXmlStreamWriter &writer) const
{
    writer.writeStartElement(QStringLiteral("f"));
    QString t;
    switch (d->type) {
    case CellFormula::ArrayType:
        t = QStringLiteral("array");
        break;
    case CellFormula::SharedType:
        t = QStringLiteral("shared");
        break;
    default:
        break;
    }
    if (!t.isEmpty())
        writer.writeAttribute(QStringLiteral("t"), t);
    if (d->reference.isValid())
        writer.writeAttribute(QStringLiteral("ref"), d->reference.toString());
    if (d->ca)
        writer.writeAttribute(QStringLiteral("ca"), QStringLiteral("1"));
    if (d->type == CellFormula::SharedType)
        writer.writeAttribute(QStringLiteral("si"), QString::number(d->si));

    if (!d->formula.isEmpty())
        writer.writeCharacters(d->formula);

    writer.writeEndElement(); //f

    return true;
}

/*!
 * \internal
 */
bool CellFormula::loadFromXml(QXmlStreamReader &reader)
{
    Q_ASSERT(reader.name() == QLatin1String("f"));
    if (!d)
        d = new CellFormulaPrivate(QString(), CellRange(), NormalType);

    QXmlStreamAttributes attributes = reader.attributes();
    QString typeString = attributes.value(QLatin1String("t")).toString();
    if (typeString == QLatin1String("array"))
        d->type = ArrayType;
    else if (typeString == QLatin1String("shared"))
        d->type = SharedType;
    else
        d->type = NormalType;

    if (attributes.hasAttribute(QLatin1String("ref"))) {
        QString refString = attributes.value(QLatin1String("ref")).toString();
        d->reference = CellRange(refString);
    }

    QString ca = attributes.value(QLatin1String("si")).toString();
    d->ca = parseXsdBoolean(ca, false);

    if (attributes.hasAttribute(QLatin1String("si")))
        d->si = attributes.value(QLatin1String("si")).toString().toInt();

    d->formula = reader.readElementText();
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
