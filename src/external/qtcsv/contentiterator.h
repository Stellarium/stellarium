#ifndef QTCSVCONTENTITERATOR_H
#define QTCSVCONTENTITERATOR_H

class QString;
class QStringList;

namespace QtCSV
{
    class AbstractData;

    // ContentIterator is a class that holds references to containers with
    // information. Its main purpose:
    // - to separate information into a chunks and
    // - to return these chunks one by one to the client.
    // You can use this class as forward-iterator that can go (only once) from
    // the beginning to the end of the data.
    // You can use this class with csv-writer class. ContentIterator will join
    // elements of one row with separator symbol and then join rows with
    // new line symbol.
    class ContentIterator
    {
    public:
        explicit ContentIterator(const AbstractData& data,
                                 const QString& separator,
                                 const QString& textDelimiter,
                                 const QStringList& header,
                                 const QStringList& footer,
                                 int chunkSize = 1000);

        ~ContentIterator() {}

        // Check if content contains information
        bool isEmpty() const;
        // Check if content still has chunks of information to return
        bool hasNext() const;
        // Get next chunk of information
        QString getNext();

    private:
        // Compose row string from values
        QString composeRow(const QStringList& values) const;

    private:
        const AbstractData& m_data;
        const QString& m_separator;
        const QString& m_textDelimiter;
        const QStringList& m_header;
        const QStringList& m_footer;
        const int m_chunkSize;
        int m_dataRow;
        bool atEnd;
    };
}

#endif // QTCSVCONTENTITERATOR_H
