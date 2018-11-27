#ifndef QTCSVREADER_H
#define QTCSVREADER_H

#include <QList>
#include <QTextCodec>

class QStringList;

namespace QtCSV
{
    class AbstractData;

    // Reader class is a file reader that work with csv-files. It needs an
    // absolute path to the csv-file that you are going to read.
    //
    // Additionally you cant specify:
    // - a separator character (or string) that is used as separator of row
    // values in this csv-file. Default separator is comma (",");
    // - text delimiter character (or string) that enclose each element in a
    // row. Typical delimiter characters: none (""), quote ("'")
    // and double quotes ("\"");
    // - text codec.
    //
    // Reader can save (or transfer) information to:
    // - QList<QStringList>, where each QStringList contains values of one row;
    // - AbstractData-based container class;
    // - AbstractProcessor-based object.
    class Reader
    {
    public:

        // AbstractProcessor is a class used to process files one line at a time
	class AbstractProcessor
        {
        public:
            explicit AbstractProcessor() {}
            virtual ~AbstractProcessor() {}

            // Preprocess one raw line from a file
            // @input:
            // line - raw line from a file
            virtual void preProcessRawLine(QString& line)
            {
                // Here you can edit line
                Q_UNUSED(line);
            }

            // Process one row worth of elements
            // @input:
            // - elements - list of row elements
            // @output:
            // bool - True if elements was processed successfully, False in case
            // of error. If process() return False, the csv-file will be stopped
            // reading
            virtual bool processRowElements(const QStringList& elements) = 0;
        };

        // Read csv-file and save it's data as strings to QList<QStringList>
        static QList<QStringList> readToList(const QString& filePath,
                        const QString& separator = QString(","),
                        const QString& textDelimiter = QString("\""),
                        QTextCodec* codec = QTextCodec::codecForName("UTF-8"));

        // Read csv-file and save it's data to AbstractData-based container
        // class
        static bool readToData(const QString& filePath,
                        AbstractData& data,
                        const QString& separator = QString(","),
                        const QString& textDelimiter = QString("\""),
                        QTextCodec* codec = QTextCodec::codecForName("UTF-8"));

        // Read csv-file and process it line-by-line
        static bool readToProcessor(const QString& filePath,
                        AbstractProcessor& processor,
                        const QString& separator = QString(","),
                        const QString& textDelimiter = QString("\""),
                        QTextCodec* codec = QTextCodec::codecForName("UTF-8"));
    };
}

#endif // QTCSVREADER_H
