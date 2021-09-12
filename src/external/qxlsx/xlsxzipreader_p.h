// xlsxzipreader_p.h

#ifndef QXLSX_XLSXZIPREADER_P_H
#define QXLSX_XLSXZIPREADER_P_H

#include <QScopedPointer>
#include <QStringList>
#include <QIODevice>

#include "xlsxglobal.h"
#include "external/qtcompress/qzipreader.h"

#if QT_VERSION >= 0x050600
 #include <QVector>
#endif

class QZipReader;

QT_BEGIN_NAMESPACE_XLSX

class  ZipReader
{
public:
    explicit ZipReader(const QString &fileName);
    explicit ZipReader(QIODevice *device);
    ~ZipReader();
    bool exists() const;
    QStringList filePaths() const;
    QByteArray fileData(const QString &fileName) const;

private:
    Q_DISABLE_COPY(ZipReader)
    void init();
    QScopedPointer<Stel::QZipReader> m_reader;
    QStringList m_filePaths;
};

QT_END_NAMESPACE_XLSX

#endif // QXLSX_XLSXZIPREADER_P_H
