// xlsxmediafile_p.h

#ifndef QXLSX_XLSXMEDIAFILE_H
#define QXLSX_XLSXMEDIAFILE_H

#include "xlsxglobal.h"

#include <QString>
#include <QByteArray>

QT_BEGIN_NAMESPACE_XLSX

class MediaFile
{
public:
    MediaFile(const QString &fileName);
    MediaFile(const QByteArray &bytes, const QString &suffix, const QString &mimeType=QString());

public:
    void set(const QByteArray &bytes, const QString &suffix, const QString &mimeType=QString());
    QString suffix() const;
    QString mimeType() const;
    QByteArray contents() const;

    bool isIndexValid() const;
    int index() const;
    void setIndex(int idx);
    QByteArray hashKey() const;

    void setFileName(const QString &name);
    QString fileName() const;

protected:
    QString m_fileName;
    QByteArray m_contents;
    QString m_suffix;
    QString m_mimeType;

    int m_index;
    bool m_indexValid;
    QByteArray m_hashKey;
};

QT_END_NAMESPACE_XLSX

#endif // QXLSX_XLSXMEDIAFILE_H
