// xlsxcolor.cpp

#include <QDataStream>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QDebug>

#include "xlsxcolor_p.h"
#include "xlsxstyles_p.h"
#include "xlsxutility_p.h"

QT_BEGIN_NAMESPACE_XLSX

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
    return val.userType() == qMetaTypeId<QColor>() && val.value<QColor>().isValid();
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
    return isRgbColor() ? val.value<QColor>() : QColor();
}

int XlsxColor::indexedColor() const
{
    return isIndexedColor() ? val.toInt() : -1;
}

QStringList XlsxColor::themeColor() const
{
    return isThemeColor() ? val.toStringList() : QStringList();
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
    const auto& attributes = reader.attributes();

    if (attributes.hasAttribute(QLatin1String("rgb"))) {
        const auto& colorString = attributes.value(QLatin1String("rgb")).toString();
        val.setValue(fromARGBString(colorString));
    } else if (attributes.hasAttribute(QLatin1String("indexed"))) {
        int index = attributes.value(QLatin1String("indexed")).toString().toInt();
        val.setValue(index);
    } else if (attributes.hasAttribute(QLatin1String("theme"))) {
        const auto& theme = attributes.value(QLatin1String("theme")).toString();
        const auto& tint = attributes.value(QLatin1String("tint")).toString();
        val.setValue(QStringList()<<theme<<tint);
    }
    return true;
}

XlsxColor::operator QVariant() const
{
    const auto& cref
#if QT_VERSION >= 0x060000 // Qt 6.0 or over
        = QMetaType::fromType<XlsxColor>();
#else
        = qMetaTypeId<XlsxColor>() ;
#endif
    return QVariant(cref, this);
}


QColor XlsxColor::fromARGBString(const QString &c)
{
    // issue #173 https://github.com/QtExcel/QXlsx/issues/173

    QColor color;
    QString strColor = "00000000";

    if ( c.length() == 8 )
    {
        strColor = c;
    }

    if ( c.length() == 6 )
    {
        strColor = QString("00") + c;
    }

#if QT_VERSION >= 0x060000 // Qt 6.0 or over
    QString strAlpha = strColor.mid(0, 2);
    int alpha = strAlpha.toInt(0, 16);
    color.setAlpha( alpha );

    QString strRed = strColor.mid(2, 2);
    int red = strRed.toInt(0, 16);
    color.setRed( red );

    QString strGreen = strColor.mid(4, 2);
    int green = strGreen.toInt(0, 16);
    color.setGreen( green );

    QString strBlue = strColor.mid(6, 2);
    int blue = strBlue.toInt(0, 16);
    color.setBlue( blue );
#else
    QStringRef strAlpha = strColor.midRef(0, 2);
    int alpha = strAlpha.toInt(0, 16);
    color.setAlpha( alpha );

    QStringRef strRed = strColor.midRef(2, 2);
    int red = strRed.toInt(0, 16);
    color.setRed( red );

    QStringRef strGreen = strColor.midRef(4, 2);
    int green = strGreen.toInt(0, 16);
    color.setGreen( green );

    QStringRef strBlue = strColor.midRef(6, 2);
    int blue = strBlue.toInt(0, 16);
    color.setBlue( blue );
#endif

    return color;
}

QString XlsxColor::toARGBString(const QColor &c)
{
#if QT_VERSION >= 0x050600 // Qt 5.6 or over
    return QString::asprintf("%02X%02X%02X%02X", c.alpha(), c.red(), c.green(), c.blue());
#else
    QString color;
    color.sprintf("%02X%02X%02X%02X", c.alpha(), c.red(), c.green(), c.blue());
    return color;
#endif
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
        dbg.nospace() << "XlsxColor(theme," << c.themeColor().join(QLatin1String(":")) << ")";

    return dbg.space();
}

#endif


QT_END_NAMESPACE_XLSX
