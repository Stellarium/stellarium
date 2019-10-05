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
#ifndef XLSXCONTENTTYPES_H
#define XLSXCONTENTTYPES_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt Xlsx API.  It exists for the convenience
// of the Qt Xlsx.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include "xlsxabstractooxmlfile.h"

#include <QString>
#include <QStringList>
#include <QMap>

class QIODevice;

namespace QXlsx {

class ContentTypes : public AbstractOOXmlFile
{
public:
    ContentTypes(CreateFlag flag);

    void addDefault(const QString &key, const QString &value);
    void addOverride(const QString &key, const QString &value);

    //Convenient funcation for addOverride()
    void addDocPropCore();
    void addDocPropApp();
    void addStyles();
    void addTheme();
    void addWorkbook();
    void addWorksheetName(const QString &name);
    void addChartsheetName(const QString &name);
    void addChartName(const QString &name);
    void addDrawingName(const QString &name);
    void addCommentName(const QString &name);
    void addTableName(const QString &name);
    void addExternalLinkName(const QString &name);
    void addSharedString();
    void addVmlName();
    void addCalcChain();
    void addVbaProject();

    void clearOverrides();

    void saveToXmlFile(QIODevice *device) const;
    bool loadFromXmlFile(QIODevice *device);
private:
    QMap<QString, QString> m_defaults;
    QMap<QString, QString> m_overrides;

    QString m_package_prefix;
    QString m_document_prefix;
};

}
#endif // XLSXCONTENTTYPES_H
