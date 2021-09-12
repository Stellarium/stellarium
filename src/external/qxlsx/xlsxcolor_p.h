// xlsxcolor_p.h

#ifndef QXLSX_XLSXCOLOR_P_H
#define QXLSX_XLSXCOLOR_P_H

#include <QtGlobal>
#include <QVariant>
#include <QColor>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>

#include "xlsxglobal.h"

QT_BEGIN_NAMESPACE_XLSX

class Styles;

class XlsxColor
{
public:
    explicit XlsxColor(const QColor &color = QColor());
    explicit XlsxColor(const QString &theme, const QString &tint=QString());
    explicit XlsxColor (int index);

    bool isThemeColor() const;
    bool isIndexedColor() const;
    bool isRgbColor() const;
    bool isInvalid() const;

    QColor rgbColor() const;
    int indexedColor() const;
    QStringList themeColor() const;

    operator QVariant() const;

    static QColor fromARGBString(const QString &c);
    static QString toARGBString(const QColor &c);

    bool saveToXml(QXmlStreamWriter &writer, const QString &node=QString()) const;
    bool loadFromXml(QXmlStreamReader &reader);

private:
    QVariant val;
};

#if !defined(QT_NO_DATASTREAM)
  QDataStream &operator<<(QDataStream &, const XlsxColor &);
  QDataStream &operator>>(QDataStream &, XlsxColor &);
#endif

#ifndef QT_NO_DEBUG_STREAM
  QDebug operator<<(QDebug dbg, const XlsxColor &c);
#endif

QT_END_NAMESPACE_XLSX

Q_DECLARE_METATYPE(QXlsx::XlsxColor)

#endif // QXLSX_XLSXCOLOR_P_H
