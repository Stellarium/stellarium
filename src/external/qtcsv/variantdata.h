#ifndef QTCSVVARIANTDATA_H
#define QTCSVVARIANTDATA_H

#include <QList>

#include "abstractdata.h"

class QVariant;
class QStringList;

namespace QtCSV
{
    // VariantData is a simple container class. It implements interface of
    // AbstractData class. It uses QVariant to hold information, so data could
    // be of almost any type - integral, strings and so on. There is only one
    // limitation - QVariant must be convertible to string (because,
    // obviously, if we want to save information to CSV file, we would need to
    // convert it to plain-text form). So don't forget to see docs of QVariant
    // before you start using this class.
    class VariantData : public AbstractData
    {
    public:
        explicit VariantData();
        VariantData(const VariantData& other);
        virtual ~VariantData();

        // Add new empty row
        virtual void addEmptyRow();
        // Add new row with one value
        bool addRow(const QVariant& value);
        // Add new row with specified values
        bool addRow(const QList<QVariant>& values);
        // Add new row with specified values (as strings)
        virtual void addRow(const QStringList& values);
        // Clear all data
        virtual void clear();
        // Insert new row at index position 'row'
        bool insertRow(const int& row,
                       const QVariant& value);

        bool insertRow(const int& row,
                       const QStringList& values);

        bool insertRow(const int& row,
                       const QList<QVariant>& values);

        // Check if there are any data
        virtual bool isEmpty() const;
        // Remove the row at index position 'row'
        void removeRow(const int& row);
        // Replace the row at index position 'row' with new row
        bool replaceRow(const int& row,
                        const QVariant& value);

        bool replaceRow(const int& row,
                        const QStringList& values);

        bool replaceRow(const int& row,
                        const QList<QVariant>& values);

        // Reserve space for 'size' rows
        void reserve(const int& size);
        // Get number of rows
        virtual int rowCount() const;
        // Get values (as list of strings) of specified row
        virtual QStringList rowValues(const int& row) const;

        bool operator==(const VariantData& other) const;

        friend bool operator!=(const VariantData& left,
                               const VariantData& right)
        {
            return !(left == right);
        }

        VariantData& operator=(const VariantData& other);

        // Add new row that would contain one value
        VariantData& operator<<(const QVariant &value);
        // Add new row with specified values
        VariantData& operator<<(const QList<QVariant> &values);
        VariantData& operator<<(const QStringList &values);

    private:
        class VariantDataPrivate;
        VariantDataPrivate* d_ptr;
    };
}

#endif // QTCSVVARIANTDATA_H
