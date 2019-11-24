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
#include "xlsxabstractsheet.h"
#include "xlsxabstractsheet_p.h"
#include "xlsxworkbook.h"

QT_BEGIN_NAMESPACE_XLSX

AbstractSheetPrivate::AbstractSheetPrivate(AbstractSheet *p, AbstractSheet::CreateFlag flag)
    : AbstractOOXmlFilePrivate(p, flag)
{
    type = AbstractSheet::ST_WorkSheet;
    sheetState = AbstractSheet::SS_Visible;
}

AbstractSheetPrivate::~AbstractSheetPrivate()
{
}

/*!
  \class AbstractSheet
  \inmodule QtXlsx
  \brief Base class for worksheet, chartsheet, etc.
*/

/*!
  \enum AbstractSheet::SheetType

  \value ST_WorkSheet
  \value ST_ChartSheet
  \omitvalue ST_DialogSheet
  \omitvalue ST_MacroSheet
*/

/*!
  \enum AbstractSheet::SheetState

  \value SS_Visible
  \value SS_Hidden
  \value SS_VeryHidden User cann't make a veryHidden sheet visible in normal way.
*/

/*!
  \fn AbstractSheet::copy(const QString &distName, int distId) const

  Copies the current sheet to a sheet called \a distName with \a distId.
  Returns the new sheet.
 */

/*!
 * \internal
 */
AbstractSheet::AbstractSheet(const QString &name, int id, Workbook *workbook, AbstractSheetPrivate *d) :
    AbstractOOXmlFile(d)
{
    d_func()->name = name;
    d_func()->id = id;
    d_func()->workbook = workbook;
}


/*!
 * Returns the name of the sheet.
 */
QString AbstractSheet::sheetName() const
{
    Q_D(const AbstractSheet);
    return d->name;
}

/*!
 * \internal
 */
void AbstractSheet::setSheetName(const QString &sheetName)
{
    Q_D(AbstractSheet);
    d->name = sheetName;
}

/*!
 * Returns the type of the sheet.
 */
AbstractSheet::SheetType AbstractSheet::sheetType() const
{
    Q_D(const AbstractSheet);
    return d->type;
}

/*!
 * \internal
 */
void AbstractSheet::setSheetType(SheetType type)
{
    Q_D(AbstractSheet);
    d->type = type;
}

/*!
 * Returns the state of the sheet.
 *
 * \sa isHidden(), isVisible(), setSheetState()
 */
AbstractSheet::SheetState AbstractSheet::sheetState() const
{
    Q_D(const AbstractSheet);
    return d->sheetState;
}

/*!
 * Set the state of the sheet to \a state.
 */
void AbstractSheet::setSheetState(SheetState state)
{
    Q_D(AbstractSheet);
    d->sheetState = state;
}

/*!
 * Returns true if the sheet is not visible, otherwise false will be returned.
 *
 * \sa sheetState(), setHidden()
 */
bool AbstractSheet::isHidden() const
{
    Q_D(const AbstractSheet);
    return d->sheetState != SS_Visible;
}

/*!
 * Returns true if the sheet is visible.
 */
bool AbstractSheet::isVisible() const
{
    return !isHidden();
}

/*!
 * Make the sheet hiden or visible based on \a hidden.
 */
void AbstractSheet::setHidden(bool hidden)
{
    Q_D(AbstractSheet);
    if (hidden == isHidden())
        return;

    d->sheetState = hidden ? SS_Hidden : SS_Visible;
}

/*!
 * Convenience function, equivalent to setHidden(! \a visible).
 */
void AbstractSheet::setVisible(bool visible)
{
    setHidden(!visible);
}

/*!
 * \internal
 */
int AbstractSheet::sheetId() const
{
    Q_D(const AbstractSheet);
    return d->id;
}

/*!
 * \internal
 */
Drawing *AbstractSheet::drawing() const
{
    Q_D(const AbstractSheet);
    return d->drawing.data();
}

/*!
 * Return the workbook
 */
Workbook *AbstractSheet::workbook() const
{
    Q_D(const AbstractSheet);
    return d->workbook;
}

QT_END_NAMESPACE_XLSX
