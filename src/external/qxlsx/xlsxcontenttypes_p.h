// xlsxcontenttypes_p.h

#ifndef XLSXCONTENTTYPES_H
#define XLSXCONTENTTYPES_H

#include <QtGlobal>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QIODevice>

#include "xlsxabstractooxmlfile.h"

QT_BEGIN_NAMESPACE_XLSX

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

QT_END_NAMESPACE_XLSX
#endif // XLSXCONTENTTYPES_H
