#ifndef QTCSVFILECHECKER_H
#define QTCSVFILECHECKER_H

#include <QString>
#include <QFileInfo>
#include <QDebug>

namespace QtCSV
{
    // Check if path to csv file is valid
    // @input:
    // - filePath - string with absolute path to file
    // - mustExist - True if file must exist, False if this is not important
    // @output:
    // - bool - True if file is OK, else False
    inline bool CheckFile(const QString& filePath,
                          const bool& mustExist = false)
    {
        QFileInfo fileInfo(filePath);
        if ( fileInfo.isAbsolute() &&
             false == fileInfo.isDir() )
        {
            if (mustExist && false == fileInfo.exists())
            {
                return false;
            }

            if ( "csv" != fileInfo.suffix() )
            {
                qDebug() << __FUNCTION__  <<
                            "Warning - file suffix is not .csv";
            }

            return true;
        }

        return false;
    }
}

#endif // QTCSVFILECHECKER_H
