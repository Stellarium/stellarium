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
#include "xlsxnumformatparser_p.h"

#include <QString>

namespace QXlsx {

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

} // namespace QXlsx
