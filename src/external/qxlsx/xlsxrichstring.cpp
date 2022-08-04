// xlsxrichstring.cpp

#include <QtGlobal>
#include <QDebug>
#include <QTextDocument>
#include <QTextFragment>

#include "xlsxrichstring.h"
#include "xlsxrichstring_p.h"
#include "xlsxformat_p.h"

QT_BEGIN_NAMESPACE_XLSX

RichStringPrivate::RichStringPrivate()
    :_dirty(true)
{

}

RichStringPrivate::RichStringPrivate(const RichStringPrivate &other)
    :QSharedData(other), fragmentTexts(other.fragmentTexts)
    ,fragmentFormats(other.fragmentFormats)
    , _idKey(other.idKey()), _dirty(other._dirty)
{

}

RichStringPrivate::~RichStringPrivate()
{

}

/*!
    \class RichString
    \inmodule QtXlsx
    \brief This class add support for the rich text string of the cell.
*/

/*!
    Constructs a null string.
 */
RichString::RichString()
    :d(new RichStringPrivate)
{
}

/*!
    Constructs a plain string with the given \a text.
*/
RichString::RichString(const QString& text)
    :d(new RichStringPrivate)
{
    addFragment(text, Format());
}

/*!
    Constructs a copy of \a other.
 */
RichString::RichString(const RichString &other)
    :d(other.d)
{

}

/*!
    Destructs the string.
 */
RichString::~RichString()
{

}

/*!
    Assigns \a other to this string and returns a reference to this string
 */
RichString &RichString::operator =(const RichString &other)
{
    this->d = other.d;
    return *this;
}

/*!
    Returns the rich string as a QVariant
*/
RichString::operator QVariant() const
{
    const auto& cref
#if QT_VERSION >= 0x060000 // Qt 6.0 or over
        = QMetaType::fromType<RichString>();
#else
        = qMetaTypeId<RichString>() ;
#endif
    return QVariant(cref, this);
}

/*!
    Returns true if this is rich text string.
 */
bool RichString::isRichString() const
{
    if (fragmentCount() > 1) //Is this enough??
        return true;
    return false;
}

/*!
    Returns true is this is an Null string.
 */
bool RichString::isNull() const
{
    return d->fragmentTexts.size() == 0;
}

/*!
    Returns true is this is an empty string.
 */
bool RichString::isEmtpy() const
{
    for (const auto& str : d->fragmentTexts) {
        if (!str.isEmpty())
            return false;
    }

    return true;
}

/*!
    Converts to plain text string.
*/
QString RichString::toPlainString() const
{
    if (isEmtpy())
        return QString();
    if (d->fragmentTexts.size() == 1)
        return d->fragmentTexts[0];

    return d->fragmentTexts.join(QString());
}

/*!
  Converts to html string
*/
QString RichString::toHtml() const
{
    //: Todo
    return QString();
}

/*!
  Replaces the entire contents of the document
  with the given HTML-formatted text in the \a text string
*/
void RichString::setHtml(const QString &text)
{
    QTextDocument doc;
    doc.setHtml(text);
    QTextBlock block = doc.firstBlock();
    QTextBlock::iterator it;
    for (it = block.begin(); !(it.atEnd()); ++it) {
        QTextFragment textFragment = it.fragment();
        if (textFragment.isValid()) {
            Format fmt;
            fmt.setFont(textFragment.charFormat().font());
            fmt.setFontColor(textFragment.charFormat().foreground().color());
            addFragment(textFragment.text(), fmt);
        }
    }
}

/*!
    Returns fragment count.
 */
int RichString::fragmentCount() const
{
    return d->fragmentTexts.size();
}

/*!
    Appends a fragment with the given \a text and \a format.
 */
void RichString::addFragment(const QString &text, const Format &format)
{
    d->fragmentTexts.append(text);
    d->fragmentFormats.append(format);
    d->_dirty = true;
}

/*!
    Returns fragment text at the position \a index.
 */
QString RichString::fragmentText(int index) const
{
    if (index < 0 || index >= fragmentCount())
        return QString();

    return d->fragmentTexts[index];
}

/*!
    Returns fragment format at the position \a index.
 */
Format RichString::fragmentFormat(int index) const
{
    if (index < 0 || index >= fragmentCount())
        return Format();

    return d->fragmentFormats[index];
}

/*!
 * \internal
 */
QByteArray RichStringPrivate::idKey() const
{
    if (_dirty) {
        RichStringPrivate *rs = const_cast<RichStringPrivate *>(this);
        QByteArray bytes;
        if (fragmentTexts.size() == 1) {
            bytes = fragmentTexts[0].toUtf8();
        } else {
            //Generate a hash value base on QByteArray ?
            bytes.append("@@QtXlsxRichString=");
            for (int i=0; i<fragmentTexts.size(); ++i) {
                bytes.append("@Text");
                bytes.append(fragmentTexts[i].toUtf8());
                bytes.append("@Format");
                if (fragmentFormats[i].hasFontData())
                    bytes.append(fragmentFormats[i].fontKey());
            }
        }
        rs->_idKey = bytes;
        rs->_dirty = false;
    }

    return _idKey;
}

/*!
    Returns true if this string \a rs1 is equal to string \a rs2;
    otherwise returns false.
 */
bool operator==(const RichString &rs1, const RichString &rs2)
{
    if (rs1.fragmentCount() != rs2.fragmentCount())
        return false;

    return rs1.d->idKey() == rs2.d->idKey();
}

/*!
    Returns true if this string \a rs1 is not equal to string \a rs2;
    otherwise returns false.
 */
bool operator!=(const RichString &rs1, const RichString &rs2)
{
    if (rs1.fragmentCount() != rs2.fragmentCount())
        return true;

    return rs1.d->idKey() != rs2.d->idKey();
}

/*!
 * \internal
 */
bool operator<(const RichString &rs1, const RichString &rs2)
{
    return rs1.d->idKey() < rs2.d->idKey();
}

/*!
    \overload
    Returns true if this string \a rs1 is equal to string \a rs2;
    otherwise returns false.
 */
bool operator ==(const RichString &rs1, const QString &rs2)
{
    if (rs1.fragmentCount() == 1 && rs1.fragmentText(0) == rs2) //format == 0
        return true;

    return false;
}

/*!
    \overload
    Returns true if this string \a rs1 is not equal to string \a rs2;
    otherwise returns false.
 */
bool operator !=(const RichString &rs1, const QString &rs2)
{
    if (rs1.fragmentCount() == 1 && rs1.fragmentText(0) == rs2) //format == 0
        return false;

    return true;
}

/*!
    \overload
    Returns true if this string \a rs1 is equal to string \a rs2;
    otherwise returns false.
 */
bool operator ==(const QString &rs1, const RichString &rs2)
{
    return rs2 == rs1;
}

/*!
    \overload
    Returns true if this string \a rs1 is not equal to string \a rs2;
    otherwise returns false.
 */
bool operator !=(const QString &rs1, const RichString &rs2)
{
    return rs2 != rs1;
}

uint qHash(const RichString &rs, uint seed) Q_DECL_NOTHROW
{
#if QT_VERSION >= QT_VERSION_CHECK( 5, 0, 0 )
   return qHash(rs.d->idKey(), seed);
#else
   Q_UNUSED(seed);
   return qHash(rs.d->idKey());
#endif
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const RichString &rs)
{
    dbg.nospace() << "QXlsx::RichString(" << rs.d->fragmentTexts << ")";
    return dbg.space();
}
#endif

QT_END_NAMESPACE_XLSX
