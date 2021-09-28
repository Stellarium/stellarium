// xlsxutility_p.h

#ifndef XLSXUTILITY_H
#define XLSXUTILITY_H

#include <QtGlobal>
#include <QObject>
#include <QString>
#include <QPoint>
#include <QString>
#include <QStringList>
#include <QColor>
#include <QDateTime>
#include <QDate>
#include <QTime>
#include <QVariant>

#include "xlsxglobal.h"

QT_BEGIN_NAMESPACE_XLSX

class CellReference;

bool parseXsdBoolean(const QString &value, bool defaultValue=false);

QStringList splitPath(const QString &path);
QString getRelFilePath(const QString &filePath);

double datetimeToNumber(const QDateTime &dt, bool is1904=false);
QVariant datetimeFromNumber(double num, bool is1904=false);
double timeToNumber(const QTime &t);

QString createSafeSheetName(const QString &nameProposal);
QString escapeSheetName(const QString &sheetName);
QString unescapeSheetName(const QString &sheetName);

bool isSpaceReserveNeeded(const QString &string);

QString convertSharedFormula(const QString &rootFormula, const CellReference &rootCell, const CellReference &cell);

QT_END_NAMESPACE_XLSX
#endif // XLSXUTILITY_H
