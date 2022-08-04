// xlsxdocument_p.h

#ifndef XLSXDOCUMENT_P_H
#define XLSXDOCUMENT_P_H

#include <QtGlobal>
#include <QMap>

#include "xlsxglobal.h"
#include "xlsxdocument.h"
#include "xlsxworkbook.h"
#include "xlsxcontenttypes_p.h"

QT_BEGIN_NAMESPACE_XLSX

class DocumentPrivate
{
    Q_DECLARE_PUBLIC(Document)
public:
    DocumentPrivate(Document *p);
    void init();

    bool loadPackage(QIODevice *device);
    bool savePackage(QIODevice *device) const;

	// copy style from one xlsx file to other
	static bool copyStyle(const QString &from, const QString &to);

    Document *q_ptr;
    const QString defaultPackageName; //default name when package name not specified
    QString packageName; //name of the .xlsx file

    QMap<QString, QString> documentProperties; //core, app and custom properties
    QSharedPointer<Workbook> workbook;
    QSharedPointer<ContentTypes> contentTypes;
	bool isLoad; 
};

QT_END_NAMESPACE_XLSX

#endif // XLSXDOCUMENT_P_H
