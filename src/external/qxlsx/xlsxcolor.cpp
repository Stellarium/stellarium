//xlsxcolor.cpp

// QXlsx
// MIT License
// https://github.com/j2doll/QXlsx
//
// QtXlsx
// https://github.com/dbzhang800/QtXlsxWriter
// http://qtxlsx.debao.me/
// MIT License

#include "xlsxcolor_p.h"
#include "xlsxstyles_p.h"
#include "xlsxutility_p.h"

#include <QDataStream>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QDebug>

namespace QXlsx {


XlsxColor::XlsxColor(const QColor &color)
{
    if (color.isValid())
        val.setValue(color);
}

XlsxColor::XlsxColor(const QString &theme, const QString &tint)
    :val(QStringList()<<theme<<tint)
{
}

XlsxColor::XlsxColor(int index)
    :val(index)
{
}

bool XlsxColor::isRgbColor() const
{
    if (val.userType() == qMetaTypeId<QColor>() && val.value<QColor>().isValid())
        return true;
    return false;
}

bool XlsxColor::isIndexedColor() const
{
    return val.userType() == QMetaType::Int;
}

bool XlsxColor::isThemeColor() const
{
    return val.userType() == QMetaType::QStringList;
}

bool XlsxColor::isInvalid() const
{
    return !val.isValid();
}

QColor XlsxColor::rgbColor() const
{
    if (isRgbColor())
        return val.value<QColor>();
    return QColor();
}

int XlsxColor::indexedColor() const
{
    if (isIndexedColor())
        return val.toInt();
    return -1;
}

QStringList XlsxColor::themeColor() const
{
    if (isThemeColor())
        return val.toStringList();
    return QStringList();
}

bool XlsxColor::saveToXml(QXmlStreamWriter &writer, const QString &node) const
{
    if (!node.isEmpty())
        writer.writeEmptyElement(node); //color, bgColor, fgColor
    else
        writer.writeEmptyElement(QStringLiteral("color"));

    if (val.userType() == qMetaTypeId<QColor>()) {
        writer.writeAttribute(QStringLiteral("rgb"), XlsxColor::toARGBString(val.value<QColor>()));
    } else if (val.userType() == QMetaType::QStringList) {
        QStringList themes = val.toStringList();
        writer.writeAttribute(QStringLiteral("theme"), themes[0]);
        if (!themes[1].isEmpty())
            writer.writeAttribute(QStringLiteral("tint"), themes[1]);
    } else if (val.userType() == QMetaType::Int) {
        writer.writeAttribute(QStringLiteral("indexed"), val.toString());
    } else {
        writer.writeAttribute(QStringLiteral("auto"), QStringLiteral("1"));
    }

    return true;
}

bool XlsxColor::loadFromXml(QXmlStreamReader &reader)
{
    QXmlStreamAttributes attributes = reader.attributes();

    if (attributes.hasAttribute(QLatin1String("rgb"))) {
        QString colorString = attributes.value(QLatin1String("rgb")).toString();
        val.setValue(fromARGBString(colorString));
    } else if (attributes.hasAttribute(QLatin1String("indexed"))) {
        int index = attributes.value(QLatin1String("indexed")).toString().toInt();
        val.setValue(index);
    } else if (attributes.hasAttribute(QLatin1String("theme"))) {
        QString theme = attributes.value(QLatin1String("theme")).toString();
        QString tint = attributes.value(QLatin1String("tint")).toString();
        val.setValue(QStringList()<<theme<<tint);
    }
    return true;
}

XlsxColor::operator QVariant() const
{
    return QVariant(qMetaTypeId<XlsxColor>(), this);
}


QColor XlsxColor::fromARGBString(const QString &c)
{
    Q_ASSERT(c.length() == 8);
    QColor color;
    color.setAlpha(c.mid(0, 2).toInt(0, 16));
    color.setRed(c.mid(2, 2).toInt(0, 16));
    color.setGreen(c.mid(4, 2).toInt(0, 16));
    color.setBlue(c.mid(6, 2).toInt(0, 16));
    return color;
}

QString XlsxColor::toARGBString(const QColor &c)
{
    QString color;
    color.sprintf("%02X%02X%02X%02X", c.alpha(), c.red(), c.green(), c.blue());
    return color;
}

#if !defined(QT_NO_DATASTREAM)
QDataStream &operator<<(QDataStream &s, const XlsxColor &color)
{
    if (color.isInvalid())
        s<<0;
    else if (color.isRgbColor())
        s<<1<<color.rgbColor();
    else if (color.isIndexedColor())
        s<<2<<color.indexedColor();
    else if (color.isThemeColor())
        s<<3<<color.themeColor();
    else
        s<<4;

    return s;
}

QDataStream &operator>>(QDataStream &s, XlsxColor &color)
{
    int marker(4);
    s>>marker;
    if (marker == 0) {
        color = XlsxColor();
    } else if (marker == 1) {
        QColor c;
        s>>c;
        color = XlsxColor(c);
    } else if (marker == 2) {
        int indexed;
        s>>indexed;
        color = XlsxColor(indexed);
    } else if (marker == 3) {
        QStringList list;
        s>>list;
        color = XlsxColor(list[0], list[1]);
    }

    return s;
}

#endif

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const XlsxColor &c)
{
    if (c.isInvalid())
        dbg.nospace() << "XlsxColor(invalid)";
    else if (c.isRgbColor())
        dbg.nospace() << c.rgbColor();
    else if (c.isIndexedColor())
        dbg.nospace() << "XlsxColor(indexed," << c.indexedColor() << ")";
    else if (c.isThemeColor())
        dbg.nospace() << "XlsxColor(theme," << c.themeColor().join(QLatin1Char(':')) << ")";

    return dbg.space();
}

#endif

} // namespace QXlsx
