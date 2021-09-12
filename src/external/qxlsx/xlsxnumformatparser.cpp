// xlsxnumformatparser.cpp

#include "xlsxnumformatparser_p.h"

#include <QtGlobal>
#include <QString>

QT_BEGIN_NAMESPACE_XLSX

bool NumFormatParser::isDateTime(const QString &formatCode)
{
    for (int i = 0; i < formatCode.length(); ++i) {
        const QChar &c = formatCode[i];

        switch (c.unicode()) {
        case '[':
            // [h], [m], [s] are valid format for time
            if (i < formatCode.length()-2 && formatCode[i+2] == QLatin1Char(']')) {
                const QChar cc = formatCode[i+1].toLower();
                if (cc == QLatin1Char('h') || cc == QLatin1Char('m') || cc == QLatin1Char('s'))
                    return true;
                i+=2;
                break;
            } else {
                // condition or color: don't care, ignore
                while (i < formatCode.length() && formatCode[i] != QLatin1Char(']'))
                    ++i;
                break;
            }

        // quoted plain text block: don't care, ignore
        case '"':
            while (i < formatCode.length()-1 && formatCode[++i] != QLatin1Char('"'))
                ;
            break;

        // escaped char: don't care, ignore
        case '\\':
            if (i < formatCode.length() - 1)
                ++i;
            break;

        // date/time can only be positive number,
        // so only the first section of the format make sense.
        case ';':
            return false;
            break;

        // days
        case 'D':
        case 'd':
        // years
        case 'Y':
        case 'y':
        // hours
        case 'H':
        case 'h':
        // seconds
        case 'S':
        case 's':
        // minutes or months, depending on context
        case 'M':
        case 'm':
            return true;

        default:
            break;
        }
    }
    return false;
}

QT_END_NAMESPACE_XLSX
