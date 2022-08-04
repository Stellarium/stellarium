// xlsxtheme_p.h

#ifndef XLSXTHEME_H
#define XLSXTHEME_H

#include <QtGlobal>
#include <QString>
#include <QIODevice>

#include "xlsxabstractooxmlfile.h"

QT_BEGIN_NAMESPACE_XLSX

class Theme : public AbstractOOXmlFile
{
public:
    Theme(CreateFlag flag);

    void saveToXmlFile(QIODevice *device) const;
    QByteArray saveToXmlData() const;
    bool loadFromXmlData(const QByteArray &data);
    bool loadFromXmlFile(QIODevice *device);

    QByteArray xmlData;
};

QT_END_NAMESPACE_XLSX

#endif // XLSXTHEME_H
