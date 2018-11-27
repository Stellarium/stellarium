#ifndef QTCSVSTRINGDATA_H
#define QTCSVSTRINGDATA_H

#include "abstractdata.h"

class QString;
class QStringList;

namespace QtCSV
{
    // StringData is a simple container class. It implements interface of
    // AbstractData class and uses strings to store information. Also it
    // provides basic functions for working with rows.
    class StringData : public AbstractData
    {
    public:
        explicit StringData();
        StringData(const StringData& other);
        virtual ~StringData();

        // Add new empty row
        virtual void addEmptyRow();
        // Add new row with one value
        void addRow(const QString& value);
        // Add new row with specified values (as strings)
        virtual void addRow(const QStringList& values);
        // Clear all data
        virtual void clear();
        // Insert new row at index position 'row'
        void insertRow(const int& row,
                       const QString& value);

        void insertRow(const int& row,
                       const QStringList& values);

        // Check if there are any data
        virtual bool isEmpty() const;
        // Remove the row at index position 'row'
        void removeRow(const int& row);
        // Replace the row at index position 'row' with new row
        void replaceRow(const int& row,
                        const QString& value);

        void replaceRow(const int& row,
                        const QStringList& values);

        // Reserve space for 'size' rows
        void reserve(const int& size);
        // Get number of rows
        virtual int rowCount() const;
        // Get values (as list of strings) of specified row
        virtual QStringList rowValues(const int& row) const;

        bool operator==(const StringData& other) const;

        friend bool operator!=(const StringData& left,
                               const StringData& right)
        {
            return !(left == right);
        }

        StringData& operator=(const StringData& other);

        // Add new row that would contain one value
        StringData& operator<<(const QString& value);
        // Add new row with specified values
        StringData& operator<<(const QStringList& values);

    private:
        class StringDataPrivate;
        StringDataPrivate* d_ptr;
    };
}

#endif // QTCSVSTRINGDATA_H
