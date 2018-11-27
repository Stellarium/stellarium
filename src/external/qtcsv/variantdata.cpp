#include "variantdata.h"

#include <QVariant>
#include <QStringList>

using namespace QtCSV;

class VariantData::VariantDataPrivate
{
public:
    // Check if all values are convertable to strings
    bool isConvertableToString(const QList<QVariant>& values) const;
    // Transform QStringList to QList<QVariant>
    QList<QVariant> toListOfVariants(const QStringList& values) const;

    QList< QList<QVariant> > m_values;
};

// Check if all values are convertable to strings
// @input:
// - values - list of values
// @output:
// - bool - True if all values are convertable to strings, False otherwise
bool VariantData::VariantDataPrivate::isConvertableToString(
        const QList<QVariant>& values) const
{
    for ( QList<QVariant>::const_iterator iter = values.constBegin();
          iter != values.constEnd(); ++iter)
    {
        if ( false == (*iter).canConvert<QString>() )
        {
            return false;
        }
    }

    return true;
}

// Transform QStringList to QList<QVariant>
// @input:
// - values - list of strings
// @output:
// - QList<QVariant> - list of the same strings, but converted to QVariants
QList<QVariant> VariantData::VariantDataPrivate::toListOfVariants(
        const QStringList& values) const
{
    QList<QVariant> list;
    for ( QStringList::const_iterator iter = values.constBegin();
          iter != values.constEnd(); ++iter)
    {
        list << QVariant(*iter);
    }

    return list;
}

VariantData::VariantData() :
    d_ptr(new VariantDataPrivate)
{

}

VariantData::VariantData(const VariantData& other) :
    d_ptr(new VariantDataPrivate)
{
    d_ptr->m_values = other.d_ptr->m_values;
}

VariantData::~VariantData()
{
    delete d_ptr;
}

// Add new empty row
void VariantData::addEmptyRow()
{
    d_ptr->m_values << QList<QVariant>();
}

// Add new row with one value
// @input:
// - value - value that is supposed to be written to the new row.
// If value is empty, empty row will be added. Value must be convertable to a
// QString!
// @output:
// - bool - True if new row was successfully added, else False
bool VariantData::addRow(const QVariant& value)
{
    if ( false == value.canConvert<QString>() )
    {
        return false;
    }

    d_ptr->m_values << (QList<QVariant>() << value);
    return true;
}

// Add new row with list of values
// @input:
// - values - list of values. If list is empty, empty row will be added.
// Values must be convertable to a QString!
// @output:
// - bool - True if new row was successfully added, else False
bool VariantData::addRow(const QList<QVariant> &values)
{
    if ( false == d_ptr->isConvertableToString(values) )
    {
        return false;
    }

    d_ptr->m_values << values;
    return true;
}

// Add new row with specified values (as strings)
// @input:
// - values - list of strings. If list is empty, empty row will be added.
void VariantData::addRow(const QStringList &values)
{
    d_ptr->m_values << d_ptr->toListOfVariants(values);
}

// Clear all data
void VariantData::clear()
{
    d_ptr->m_values.clear();
}

// Insert new row at index position 'row'.
// @input:
// - row - index of row. If 'row' is 0, the value will be set as first row.
// If 'row' is >= rowCount(), the value will be added as new last row.
// - value - value that is supposed to be written to the new row. Value must be
// convertable to a QString!
// @output:
// - bool - True if row was inserted, False otherwise
bool VariantData::insertRow(const int& row,
                            const QVariant& value)
{
    return insertRow(row, (QList<QVariant>() << value));
}

// Insert new row at index position 'row'.
// @input:
// - row - index of row. If 'row' is 0, the value will be set as first row.
// If 'row' is >= rowCount(), the values will be added as new last row.
// - values - list of strings that are supposed to be written to the new row
// @output:
// - bool - True if row was inserted, False otherwise
bool VariantData::insertRow(const int& row,
                            const QStringList& values)
{
    return insertRow(row, d_ptr->toListOfVariants(values));
}

// Insert new row at index position 'row'.
// @input:
// - row - index of row. If 'row' is 0, the value will be set as first row.
// If 'row' is >= rowCount(), the values will be added as new last row.
// - values - list of values that are supposed to be written to the new row.
// Values must be convertable to a QString!
// @output:
// - bool - True if row was inserted, False otherwise
bool VariantData::insertRow(const int& row,
                            const QList<QVariant>& values)
{
    if ( false == d_ptr->isConvertableToString(values) )
    {
        return false;
    }

    d_ptr->m_values.insert(row, values);
    return true;
}

// Check if there are any rows
// @output:
// - bool - True if there are any rows, else False
bool VariantData::isEmpty() const
{
    return d_ptr->m_values.isEmpty();
}

// Remove the row at index position 'row'
// @input:
// - row - index of row to remove. 'row' must be a valid index position
// (i.e., 0 <= row < rowCount()). Otherwise function will do nothing.
void VariantData::removeRow(const int& row)
{
    d_ptr->m_values.removeAt(row);
}

// Replace the row at index position 'row' with new row.
// @input:
// - row - index of row that should be replaced. 'row' must be
// a valid index position (i.e., 0 <= row < rowCount()).
// - value - value that is supposed to be written instead of the 'old' values.
// Value must be convertable to QString!
// @output:
// - bool - True if row was replaced, else False
bool VariantData::replaceRow(const int& row,
                             const QVariant& value)
{
    return replaceRow(row, (QList<QVariant>() << value));
}

// Replace the row at index position 'row' with new row.
// @input:
// - row - index of row that should be replaced. 'row' must be
// a valid index position (i.e., 0 <= row < rowCount()).
// - values - values that are supposed to be written instead of the 'old'
// values.
// @output:
// - bool - True if row was replaced, else False
bool VariantData::replaceRow(const int& row,
                             const QStringList& values)
{
    return replaceRow(row, d_ptr->toListOfVariants(values));
}

// Replace the row at index position 'row' with new row.
// @input:
// - row - index of row that should be replaced. 'row' must be
// a valid index position (i.e., 0 <= row < rowCount()).
// - values - values that are supposed to be written instead of the 'old'
// values. Values must be convertable to a QString!
// @output:
// - bool - True if row was replaced, else False
bool VariantData::replaceRow(const int& row,
                             const QList<QVariant>& values)
{
    if ( false == d_ptr->isConvertableToString(values) )
    {
        return false;
    }

    d_ptr->m_values.replace(row, values);
    return true;
}

// Reserve space for 'size' rows.
// @input:
// - size - number of rows to reserve in memory. If 'size' is smaller than the
// current number of rows, function will do nothing.
void VariantData::reserve(const int& size)
{
    d_ptr->m_values.reserve(size);
}

// Get number of rows
// @output:
// - int - current number of rows
int VariantData::rowCount() const
{
    return d_ptr->m_values.size();
}

// Get values (as list of strings) of specified row
// @input:
// - row - valid number of the row
// @output:
// - QStringList - values of the row. If row have invalid value, function will
// return empty QStringList.
QStringList VariantData::rowValues(const int& row) const
{
    if ( row < 0 || rowCount() <= row )
    {
        return QStringList();
    }

    QStringList values;
    for ( int i = 0; i < d_ptr->m_values.at(row).size(); ++i )
    {
        values << d_ptr->m_values.at(row).at(i).toString();
    }

    return values;
}

bool VariantData::operator==(const VariantData& other) const
{
    return d_ptr->m_values == other.d_ptr->m_values;
}

VariantData& VariantData::operator=(const VariantData& other)
{
    VariantData tmp(other);
    std::swap(d_ptr, tmp.d_ptr);
    return *this;
}

// Add new row that would contain one value
VariantData& VariantData::operator<<(const QVariant& value)
{
    this->addRow(value);
    return *this;
}

// Add new row with specified values
VariantData& VariantData::operator<<(const QList<QVariant>& values)
{
    this->addRow(values);
    return *this;
}

// Add new row with specified values
VariantData& VariantData::operator<<(const QStringList& values)
{
    this->addRow(values);
    return *this;
}
