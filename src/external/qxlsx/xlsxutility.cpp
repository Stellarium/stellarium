// xlsxutility.cpp

#include "xlsxutility_p.h"
#include "xlsxcellreference.h"

#include <QString>
#include <QPoint>
#if QT_VERSION >= QT_VERSION_CHECK( 5, 0, 0 )
#include <QRegularExpression>
#else
#include <QRegExp>
#endif
#include <QMap>
#include <QStringList>
#include <QColor>
#include <QDateTime>
#include <QDebug>

#include <cmath>
#include <string>

QT_BEGIN_NAMESPACE_XLSX

bool parseXsdBoolean(const QString &value, bool defaultValue)
{
    if (value == QLatin1String("1") || value == QLatin1String("true"))
        return true;
    if (value == QLatin1String("0") || value == QLatin1String("false"))
        return false;
    return defaultValue;
}

QStringList splitPath(const QString &path)
{
    int idx = path.lastIndexOf(QLatin1Char('/'));
    if (idx == -1)
        return { QStringLiteral("."), path };

    return { path.left(idx), path.mid(idx+1) };
}

/*
 * Return the .rel file path based on filePath
 */
QString getRelFilePath(const QString &filePath)
{
    QString ret;

    int idx = filePath.lastIndexOf(QLatin1Char('/'));
    if (idx == -1) // not found
    {
        // return QString();

        // dev34
        ret = QLatin1String("_rels/") + QStringLiteral("%0.rels").arg(filePath);
        return ret;
    }

    ret = QString( filePath.left(idx) + QLatin1String("/_rels/") +  filePath.mid(idx+1) + QLatin1String(".rels"));
    return ret;
}

double datetimeToNumber(const QDateTime &dt, bool is1904)
{
    //Note, for number 0, Excel2007 shown as 1900-1-0, which should be 1899-12-31
    QDateTime epoch(is1904 ? QDate(1904, 1, 1): QDate(1899, 12, 31), QTime(0,0));

    double excel_time = epoch.msecsTo(dt) / (1000*60*60*24.0);

#if QT_VERSION >= 0x050200
    if (dt.isDaylightTime())    // Add one hour if the date is Daylight
        excel_time += 1.0 / 24.0;
#endif

    if (!is1904 && excel_time > 59) {//31+28
        //Account for Excel erroneously treating 1900 as a leap year.
        excel_time += 1;
    }

    return excel_time;
}

double timeToNumber(const QTime &time)
{
    return QTime(0,0).msecsTo(time) / (1000*60*60*24.0);
}

QVariant datetimeFromNumber(double num, bool is1904)
{
    QDateTime dtRet; // return value

    if (!is1904 && num > 60) // for mac os excel
    {
        num = num - 1;
    }

    qint64 msecs = static_cast<qint64>(num * 1000*60*60*24.0 + 0.5);
    QDateTime epoch(is1904 ? QDate(1904, 1, 1): QDate(1899, 12, 31), QTime(0,0));
    QDateTime dtOld = epoch.addMSecs(msecs);
    dtRet = dtOld;

#if QT_VERSION >= 0x050200
    // Remove one hour to see whether the date is Daylight
    QDateTime dtNew = dtRet.addMSecs( -3600000 ); // issue102
    if ( dtNew.isDaylightTime() )
    {
        dtRet = dtNew;
    }
#endif

    float whole = 0;
    float fractional = std::modf(num, &whole);

    if ( num < double(1) )
    {
        // only time
        QTime t = dtRet.time();
        return QVariant(t);
    }

    if ( fractional == 0.0 )
    {
        // only date
        QDate onlyDT = dtRet.date();
        return QVariant(onlyDT);
    }

    return QVariant(dtRet);
}

/*
  Creates a valid sheet name
    minimum length is 1
    maximum length is 31
    doesn't contain special chars: / \ ? * ] [ :
    Sheet names must not begin or end with ' (apostrophe)

  Invalid characters are replaced by one space character ' '.
 */
QString createSafeSheetName(const QString &nameProposal)
{
    if (nameProposal.isEmpty())
        return QString();

    QString ret = nameProposal;
    if (nameProposal.length() > 2 && nameProposal.startsWith(QLatin1Char('\'')) && nameProposal.endsWith(QLatin1Char('\'')))
        ret = unescapeSheetName(ret);

    //Replace invalid chars with space.
#if QT_VERSION >= QT_VERSION_CHECK( 5, 0, 0 )
    if (nameProposal.contains(QRegularExpression(QStringLiteral("[/\\\\?*\\][:]"))))
        ret.replace(QRegularExpression(QStringLiteral("[/\\\\?*\\][:]")), QStringLiteral(" "));
#else
    if (nameProposal.contains(QRegExp(QLatin1String("[/\\\\?*\\][:]"))))
        ret.replace(QRegExp(QLatin1String("[/\\\\?*\\][:]")), QLatin1String(" "));
#endif
    if (ret.startsWith(QLatin1Char('\'')))
        ret[0] = QLatin1Char(' ');
    if (ret.endsWith(QLatin1Char('\'')))
        ret[ret.size()-1] = QLatin1Char(' ');

    if (ret.size() > 31)
        ret = ret.left(31);
    return ret;
}

/*
 * When sheetName contains space or apostrophe, escaped is needed by cellFormula/definedName/chartSerials.
 */
QString escapeSheetName(const QString &sheetName)
{
    //Already escaped.
    Q_ASSERT(!sheetName.startsWith(QLatin1Char('\'')) && !sheetName.endsWith(QLatin1Char('\'')));

    //These is no need to escape
#if QT_VERSION >= QT_VERSION_CHECK( 5, 0, 0 )
    if (!sheetName.contains(QRegularExpression(QStringLiteral("[ +\\-,%^=<>'&]"))))
        return sheetName;
#else
    if (!sheetName.contains(QRegExp(QLatin1String("[ +\\-,%^=<>'&]"))))
        return sheetName;
#endif

    //OK, escape is needed.
    QString name = sheetName;
    name.replace(QLatin1Char('\''), QLatin1String("\'\'"));
    return QLatin1Char('\'') + name + QLatin1Char('\'');
}

/*
 */
QString unescapeSheetName(const QString &sheetName)
{
    Q_ASSERT(sheetName.length() > 2 && sheetName.startsWith(QLatin1Char('\'')) && sheetName.endsWith(QLatin1Char('\'')));

    QString name = sheetName.mid(1, sheetName.length()-2);
    name.replace(QLatin1String("\'\'"), QLatin1String("\'"));
    return name;
}

/*
 * whether the string s starts or ends with space
 */
bool isSpaceReserveNeeded(const QString &s)
{
    QString spaces(QStringLiteral(" \t\n\r"));
    return !s.isEmpty() && (spaces.contains(s.at(0))||spaces.contains(s.at(s.length()-1)));
}

/*
 * Convert shared formula for non-root cells.
 *
 * For example, if "B1:B10" have shared formula "=A1*A1", this function will return "=A2*A2"
 * for "B2" cell, "=A3*A3" for "B3" cell, etc.
 *
 * Note, the formula "=A1*A1" for B1 can also be written as "=RC[-1]*RC[-1]", which is the same
 * for all other cells. In other words, this formula is shared.
 *
 * For long run, we need a formula parser.
 */
QString convertSharedFormula(const QString &rootFormula, const CellReference &rootCell, const CellReference &cell)
{
    Q_UNUSED(rootCell)
    Q_UNUSED(cell)
    //Find all the "$?[A-Z]+$?[0-9]+" patterns in the rootFormula.
    QVector<std::pair<QString, int> > segments;

    QString segment;
    bool inQuote = false;
    enum RefState{INVALID, PRE_AZ, AZ, PRE_09, _09};
    RefState refState = INVALID;
    int refFlag = 0; // 0x00, 0x01, 0x02, 0x03 ==> A1, $A1, A$1, $A$1
    for (QChar ch : rootFormula) {
        if (inQuote) {
            segment.append(ch);
            if (ch == QLatin1Char('"'))
                inQuote = false;
        } else {
            if (ch == QLatin1Char('"')) {
                inQuote = true;
                refState = INVALID;
                segment.append(ch);
            } else if (ch == QLatin1Char('$')) {
                if (refState == AZ) {
                    segment.append(ch);
                    refState = PRE_09;
                    refFlag |= 0x02;
                } else {
                    segments.append(std::make_pair(segment, refState==_09 ? refFlag : -1 ));
                    segment = QString(ch); //Start new segment.
                    refState = PRE_AZ;
                    refFlag = 0x01;
                }
            } else if (ch >= QLatin1Char('A') && ch <=QLatin1Char('Z')) {
                if (refState == PRE_AZ || refState == AZ) {
                    segment.append(ch);
                } else {
                    segments.append(std::make_pair(segment, refState==_09 ? refFlag : -1 ));
                    segment = QString(ch); //Start new segment.
                    refFlag = 0x00;
                }
                refState = AZ;
            } else if (ch >= QLatin1Char('0') && ch <=QLatin1Char('9')) {
                segment.append(ch);

                if (refState == AZ || refState == PRE_09 || refState == _09)
                    refState = _09;
                else
                    refState = INVALID;
            } else {
                if (refState == _09) {
                    segments.append(std::make_pair(segment, refFlag ));
                    segment = QString(ch); //Start new segment.
                } else {
                    segment.append(ch);
                }
                refState = INVALID;
            }
        }
    }

    if (!segment.isEmpty())
        segments.append(std::make_pair(segment, refState==_09 ? refFlag : -1 ));

    //Replace "A1", "$A1", "A$1" segment with proper one.
    QStringList result;
    for (const auto &p : segments) {
        //qDebug()<<p.first<<p.second;
        if (p.second != -1 && p.second != 3) {
            CellReference oldRef(p.first);
            int row = p.second & 0x02 ? oldRef.row() : oldRef.row()-rootCell.row()+cell.row();
            int col = p.second & 0x01 ? oldRef.column() : oldRef.column()-rootCell.column()+cell.column();
            result.append(CellReference(row, col).toString(p.second & 0x02, p.second & 0x01));
        } else {
            result.append(p.first);
        }
    }

    //OK
    return result.join(QString());
}

QT_END_NAMESPACE_XLSX
