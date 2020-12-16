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
#ifndef XLSXWORKBOOK_H
#define XLSXWORKBOOK_H

#include "xlsxglobal.h"
#include "xlsxabstractooxmlfile.h"
#include "xlsxabstractsheet.h"
#include <QList>
#include <QImage>
#include <QSharedPointer>

class QIODevice;

QT_BEGIN_NAMESPACE_XLSX

class SharedStrings;
class Styles;
class Drawing;
class Document;
class Theme;
class Relationships;
class DocumentPrivate;
class MediaFile;
class Chart;
class Chartsheet;
class Worksheet;

class WorkbookPrivate;
class Workbook : public AbstractOOXmlFile
{
    Q_DECLARE_PRIVATE(Workbook)
public:
    ~Workbook();

    int sheetCount() const;
    AbstractSheet *sheet(int index) const;

    AbstractSheet *addSheet(const QString &name = QString(), AbstractSheet::SheetType type = AbstractSheet::ST_WorkSheet);
    AbstractSheet *insertSheet(int index, const QString &name = QString(), AbstractSheet::SheetType type = AbstractSheet::ST_WorkSheet);
    bool renameSheet(int index, const QString &name);
    bool deleteSheet(int index);
    bool copySheet(int index, const QString &newName=QString());
    bool moveSheet(int srcIndex, int distIndex);

    AbstractSheet *activeSheet() const;
    bool setActiveSheet(int index);

//    void addChart();
    bool defineName(const QString &name, const QString &formula, const QString &comment=QString(), const QString &scope=QString());
    bool isDate1904() const;
    void setDate1904(bool date1904);
    bool isStringsToNumbersEnabled() const;
    void setStringsToNumbersEnabled(bool enable=true);
    bool isStringsToHyperlinksEnabled() const;
    void setStringsToHyperlinksEnabled(bool enable=true);
    bool isHtmlToRichStringEnabled() const;
    void setHtmlToRichStringEnabled(bool enable=true);
    QString defaultDateFormat() const;
    void setDefaultDateFormat(const QString &format);

    //internal used member
    void addMediaFile(QSharedPointer<MediaFile> media, bool force=false);
    QList<QSharedPointer<MediaFile> > mediaFiles() const;
    void addChartFile(QSharedPointer<Chart> chartFile);
    QList<QSharedPointer<Chart> > chartFiles() const;

private:
    friend class Worksheet;
    friend class Chartsheet;
    friend class WorksheetPrivate;
    friend class Document;
    friend class DocumentPrivate;

    Workbook(Workbook::CreateFlag flag);

    void saveToXmlFile(QIODevice *device) const;
    bool loadFromXmlFile(QIODevice *device);

    SharedStrings *sharedStrings() const;
    Styles *styles();
    Theme *theme();
    QList<QImage> images();
    QList<Drawing *> drawings();
    QList<QSharedPointer<AbstractSheet> > getSheetsByTypes(AbstractSheet::SheetType type) const;
    QStringList worksheetNames() const;
    AbstractSheet *addSheet(const QString &name, int sheetId, AbstractSheet::SheetType type = AbstractSheet::ST_WorkSheet);
};

QT_END_NAMESPACE_XLSX

#endif // XLSXWORKBOOK_H
