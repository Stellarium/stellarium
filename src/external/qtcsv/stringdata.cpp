#include "stringdata.h"

#include <QList>
#include <QStringList>

using namespace QtCSV;

class StringData::StringDataPrivate
{
public:
    QList<QStringList> m_values;
};

StringData::StringData() :
    d_ptr(new StringDataPrivate)
{

}

StringData::StringData(const StringData& other) :
    d_ptr(new StringDataPrivate)
{
    d_ptr->m_values = other.d_ptr->m_values;
}

StringData::~StringData()
{
    delete d_ptr;
}

// Add new empty row
void StringData::addEmptyRow()
{
    d_ptr->m_values << QStringList();
}

// Add new row with one value
// @input:
// - value - value that is supposed to be written to the new row
void StringData::addRow(const QString& value)
{
    d_ptr->m_values << (QStringList() << value);
}

// Add new row with specified values (as strings)
// @input:
// - values - list of strings. If list is empty, it will be interpreted
// as empty line
void StringData::addRow(const QStringList& values)
{
    d_ptr->m_values << values;
}

// Clear all data
void StringData::clear()
{
    d_ptr->m_values.clear();
}

// Insert new row at index position 'row'.
// @input:
// - row - index of row. If 'row' is 0, the value will be set as first row.
// If 'row' is >= rowCount(), the value will be added as new last row.
// - value - value that is supposed to be written to the new row
void StringData::insertRow(const int& row,
                           const QString& value)
{
    insertRow(row, (QStringList() << value));
}

// Insert new row at index position 'row'.
// @input:
// - row - index of row. If 'row' is 0, the values will be set as first row.
// If 'row' is >= rowCount(), the values will be added as new last row.
// - values - list of strings
void StringData::insertRow(const int& row,
                           const QStringList& values)
{
    d_ptr->m_values.insert(row, values);
}

// Check if there are any rows
// @output:
// - bool - True if there are any rows, else False
bool StringData::isEmpty() const
{
    return d_ptr->m_values.isEmpty();
}

// Remove the row at index position 'row'.
// @input:
// - row - index of row to remove. 'row' must be a valid index position
// (i.e., 0 <= row < rowCount()). Otherwise function will do nothing.
void StringData::removeRow(const int& row)
{
    d_ptr->m_values.removeAt(row);
}

// Replace the row at index position 'row' with new row.
// @input:
// - row - index of row that should be replaced. 'row' must be
// a valid index position (i.e., 0 <= row < rowCount()).
// - value - value that is supposed to be written instead of the 'old' values
void StringData::replaceRow(const int& row,
                            const QString& value)
{
    replaceRow(row, (QStringList() << value));
}

// Replace the row at index position 'row' with new row.
// @input:
// - row - index of row that should be replaced. 'row' must be
// a valid index position (i.e., 0 <= row < rowCount()).
// - values - list of strings that is supposed to be written instead of the
// 'old' values
void StringData::replaceRow(const int& row,
                            const QStringList& values)
{
    d_ptr->m_values.replace(row, values);
}

// Reserve space for 'size' rows.
// @input:
// - size - number of rows to reserve in memory. If 'size' is smaller than the
// current number of rows, function will do nothing.
void StringData::reserve(const int& size)
{
    d_ptr->m_values.reserve(size);
}

// Get number of rows
// @output:
// - int - current number of rows
int StringData::rowCount() const
{
    return d_ptr->m_values.size();
}

// Get values (as list of strings) of specified row
// @input:
// - row - valid number of row
// @output:
// - QStringList - values of row. If row is invalid number, function will
// return empty QStringList.
QStringList StringData::rowValues(const int& row) const
{
    if ( row < 0 || rowCount() <= row )
    {
        return QStringList();
    }

    return d_ptr->m_values.at(row);
}

bool StringData::operator==(const StringData& other) const
{
    return d_ptr->m_values == other.d_ptr->m_values;
}

StringData& StringData::operator=(const StringData& other)
{
    StringData tmp(other);
    std::swap(d_ptr, tmp.d_ptr);
    return *this;
}

// Add new row that would contain one value
StringData& StringData::operator<<(const QString& value)
{
    this->addRow(value);
    return *this;
}

// Add new row with specified values
StringData& StringData::operator<<(const QStringList& values)
{
    this->addRow(values);
    return *this;
}
