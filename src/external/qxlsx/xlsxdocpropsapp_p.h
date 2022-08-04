// xlsxdocpropsapp_p.h

#ifndef XLSXDOCPROPSAPP_H
#define XLSXDOCPROPSAPP_H

#include <QList>
#include <QStringList>
#include <QMap>

#include "xlsxglobal.h"
#include "xlsxabstractooxmlfile.h"

class QIODevice;

QT_BEGIN_NAMESPACE_XLSX

class  DocPropsApp : public AbstractOOXmlFile
{
public:
    DocPropsApp(CreateFlag flag);
    
    void addPartTitle(const QString &title);
    void addHeadingPair(const QString &name, int value);

    bool setProperty(const QString &name, const QString &value);
    QString property(const QString &name) const;
    QStringList propertyNames() const;

    void saveToXmlFile(QIODevice *device) const;
    bool loadFromXmlFile(QIODevice *device);

private:
    QStringList m_titlesOfPartsList;
    QList<std::pair<QString, int> > m_headingPairsList;
    QMap<QString, QString> m_properties;
};

QT_END_NAMESPACE_XLSX

#endif // XLSXDOCPROPSAPP_H
