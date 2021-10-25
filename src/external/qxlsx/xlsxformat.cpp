// xlsxformat.cpp

#include <QtGlobal>
#include <QDataStream>
#include <QDebug>

#include "xlsxformat.h"
#include "xlsxformat_p.h"
#include "xlsxcolor_p.h"
#include "xlsxnumformatparser_p.h"

QT_BEGIN_NAMESPACE_XLSX

FormatPrivate::FormatPrivate()
	: dirty(true)
	, font_dirty(true), font_index_valid(false), font_index(0)
	, fill_dirty(true), fill_index_valid(false), fill_index(0)
	, border_dirty(true), border_index_valid(false), border_index(0)
	, xf_index(-1), xf_indexValid(false)
	, is_dxf_fomat(false), dxf_index(-1), dxf_indexValid(false)
	, theme(0)
{
}

FormatPrivate::FormatPrivate(const FormatPrivate &other)
	: QSharedData(other)
	, dirty(other.dirty), formatKey(other.formatKey)
	, font_dirty(other.font_dirty), font_index_valid(other.font_index_valid), font_key(other.font_key), font_index(other.font_index)
	, fill_dirty(other.fill_dirty), fill_index_valid(other.fill_index_valid), fill_key(other.fill_key), fill_index(other.fill_index)
	, border_dirty(other.border_dirty), border_index_valid(other.border_index_valid), border_key(other.border_key), border_index(other.border_index)
	, xf_index(other.xf_index), xf_indexValid(other.xf_indexValid)
	, is_dxf_fomat(other.is_dxf_fomat), dxf_index(other.dxf_index), dxf_indexValid(other.dxf_indexValid)
	, theme(other.theme)
	, properties(other.properties)
{

}

FormatPrivate::~FormatPrivate()
{

}

/*!
 * \class Format
 * \inmodule QtXlsx
 * \brief Providing the methods and properties that are available for formatting cells in Excel.
 */

/*!
 * \enum Format::FontScript
 *
 * The enum type defines the type of font script.
 *
 * \value FontScriptNormal normal
 * \value FontScriptSuper super script
 * \value FontScriptSub sub script
 */


/*!
 * \enum Format::FontUnderline
 *
 * The enum type defines the type of font underline.
 *
 * \value FontUnderlineNone
 * \value FontUnderlineSingle
 * \value FontUnderlineDouble
 * \value FontUnderlineSingleAccounting
 * \value FontUnderlineDoubleAccounting
 */

/*!
 * \enum Format::HorizontalAlignment
 *
 * The enum type defines the type of horizontal alignment.
 *
 * \value AlignHGeneral
 * \value AlignLeft
 * \value AlignHCenter
 * \value AlignRight
 * \value AlignHFill
 * \value AlignHJustify
 * \value AlignHMerge
 * \value AlignHDistributed
 */

/*!
 * \enum Format::VerticalAlignment
 *
 * The enum type defines the type of vertical alignment.
 *
 * \value AlignTop,
 * \value AlignVCenter,
 * \value AlignBottom,
 * \value AlignVJustify,
 * \value AlignVDistributed
 */

/*!
 * \enum Format::BorderStyle
 *
 * The enum type defines the type of font underline.
 *
 * \value BorderNone
 * \value BorderThin
 * \value BorderMedium
 * \value BorderDashed
 * \value BorderDotted
 * \value BorderThick
 * \value BorderDouble
 * \value BorderHair
 * \value BorderMediumDashed
 * \value BorderDashDot
 * \value BorderMediumDashDot
 * \value BorderDashDotDot
 * \value BorderMediumDashDotDot
 * \value BorderSlantDashDot
*/

/*!
 * \enum Format::DiagonalBorderType
 *
 * The enum type defines the type of diagonal border.
 *
 * \value DiagonalBorderNone
 * \value DiagonalBorderDown
 * \value DiagonalBorderUp
 * \value DiagnoalBorderBoth
 */

/*!
 * \enum Format::FillPattern
 *
 * The enum type defines the type of fill.
 *
 * \value PatternNone
 * \value PatternSolid
 * \value PatternMediumGray
 * \value PatternDarkGray
 * \value PatternLightGray
 * \value PatternDarkHorizontal
 * \value PatternDarkVertical
 * \value PatternDarkDown
 * \value PatternDarkUp
 * \value PatternDarkGrid
 * \value PatternDarkTrellis
 * \value PatternLightHorizontal
 * \value PatternLightVertical
 * \value PatternLightDown
 * \value PatternLightUp
 * \value PatternLightTrellis
 * \value PatternGray125
 * \value PatternGray0625
 * \value PatternLightGrid
 */

/*!
 *  Creates a new invalid format.
 */
Format::Format()
{
	//The d pointer is initialized with a null pointer
}

/*!
   Creates a new format with the same attributes as the \a other format.
 */
Format::Format(const Format &other)
	:d(other.d)
{

}

/*!
   Assigns the \a other format to this format, and returns a
   reference to this format.
 */
Format &Format::operator =(const Format &other)
{
	d = other.d;
	return *this;
}

/*!
 * Destroys this format.
 */
Format::~Format()
{
}

/*!
 * Returns the number format identifier.
 */
int Format::numberFormatIndex() const
{
	return intProperty(FormatPrivate::P_NumFmt_Id, 0);
}

/*!
 * Set the number format identifier. The \a format
 * must be a valid built-in number format identifier
 * or the identifier of a custom number format.
 */
void Format::setNumberFormatIndex(int format)
{
	setProperty(FormatPrivate::P_NumFmt_Id, format);
	clearProperty(FormatPrivate::P_NumFmt_FormatCode);
}

/*!
 * Returns the number format string.
 * \note for built-in number formats, this may
 * return an empty string.
 */
QString Format::numberFormat() const
{
	return stringProperty(FormatPrivate::P_NumFmt_FormatCode);
}

/*!
 * Set number \a format.
 * http://office.microsoft.com/en-001/excel-help/create-a-custom-number-format-HP010342372.aspx
 */
void Format::setNumberFormat(const QString &format)
{
	if (format.isEmpty())
		return;
	setProperty(FormatPrivate::P_NumFmt_FormatCode, format);
	clearProperty(FormatPrivate::P_NumFmt_Id); //numFmt id must be re-generated.
}

/*!
 * Returns whether the number format is probably a dateTime or not
 */
bool Format::isDateTimeFormat() const
{
	//NOTICE:

	if (hasProperty(FormatPrivate::P_NumFmt_FormatCode)) 
	{
		//Custom numFmt, so
		//Gauss from the number string
		return NumFormatParser::isDateTime(numberFormat());
	} 
	else if (hasProperty(FormatPrivate::P_NumFmt_Id))
	{
		//Non-custom numFmt
		int idx = numberFormatIndex();

		//Is built-in date time number id?
		if ((idx >= 14 && idx <= 22) || (idx >= 45 && idx <= 47))
			return true;

		if ((idx >= 27 && idx <= 36) || (idx >= 50 && idx <= 58)) //Used in CHS\CHT\JPN\KOR
			return true;
	}

	return false;
}

/*!
	\internal
	Set a custom num \a format with the given \a id.
 */
void Format::setNumberFormat(int id, const QString &format)
{
	setProperty(FormatPrivate::P_NumFmt_Id, id);
	setProperty(FormatPrivate::P_NumFmt_FormatCode, format);
}

/*!
	\internal
	Called by styles to fix the numFmt
 */
void Format::fixNumberFormat(int id, const QString &format)
{
	setProperty(FormatPrivate::P_NumFmt_Id, id, 0, false);
	setProperty(FormatPrivate::P_NumFmt_FormatCode, format, QString(), false);
}

/*!
	\internal
	Return true if the format has number format.
 */
bool Format::hasNumFmtData() const
{
	if (!d)
		return false;

    if ( hasProperty(FormatPrivate::P_NumFmt_Id) ||
         hasProperty(FormatPrivate::P_NumFmt_FormatCode) )
    {
		return true;
	}
	return false;
}

/*!
 * Return the size of the font in points.
 */
int Format::fontSize() const
{
	return intProperty(FormatPrivate::P_Font_Size);
}

/*!
 * Set the \a size of the font in points.
 */
void Format::setFontSize(int size)
{
	setProperty(FormatPrivate::P_Font_Size, size, 0);
}

/*!
 * Return whether the font is italic.
 */
bool Format::fontItalic() const
{
	return boolProperty(FormatPrivate::P_Font_Italic);
}

/*!
 * Turn on/off the italic font based on \a italic.
 */
void Format::setFontItalic(bool italic)
{
	setProperty(FormatPrivate::P_Font_Italic, italic, false);
}

/*!
 * Return whether the font is strikeout.
 */
bool Format::fontStrikeOut() const
{
	return boolProperty(FormatPrivate::P_Font_StrikeOut);
}

/*!
 * Turn on/off the strikeOut font based on \a strikeOut.
 */
void Format::setFontStrikeOut(bool strikeOut)
{
	setProperty(FormatPrivate::P_Font_StrikeOut, strikeOut, false);
}

/*!
 * Return the color of the font.
 */
QColor Format::fontColor() const
{
	if (hasProperty(FormatPrivate::P_Font_Color))
		return colorProperty(FormatPrivate::P_Font_Color);
	return QColor();
}

/*!
 * Set the \a color of the font.
 */
void Format::setFontColor(const QColor &color)
{
	setProperty(FormatPrivate::P_Font_Color, XlsxColor(color), XlsxColor());
}

/*!
 * Return whether the font is bold.
 */
bool Format::fontBold() const
{
	return boolProperty(FormatPrivate::P_Font_Bold);
}

/*!
 * Turn on/off the bold font based on the given \a bold.
 */
void Format::setFontBold(bool bold)
{
	setProperty(FormatPrivate::P_Font_Bold, bold, false);
}

/*!
 * Return the script style of the font.
 */
Format::FontScript Format::fontScript() const
{
	return static_cast<Format::FontScript>(intProperty(FormatPrivate::P_Font_Script));
}

/*!
 * Set the script style of the font to \a script.
 */
void Format::setFontScript(FontScript script)
{
	setProperty(FormatPrivate::P_Font_Script, script, FontScriptNormal);
}

/*!
 * Return the underline style of the font.
 */
Format::FontUnderline Format::fontUnderline() const
{
	return static_cast<Format::FontUnderline>(intProperty(FormatPrivate::P_Font_Underline));
}

/*!
 * Set the underline style of the font to \a underline.
 */
void Format::setFontUnderline(FontUnderline underline)
{
	setProperty(FormatPrivate::P_Font_Underline, underline, FontUnderlineNone);
}

/*!
 * Return whether the font is outline.
 */
bool Format::fontOutline() const
{
	return boolProperty(FormatPrivate::P_Font_Outline);
}

/*!
 * Turn on/off the outline font based on \a outline.
 */
void Format::setFontOutline(bool outline)
{
	setProperty(FormatPrivate::P_Font_Outline, outline, false);
}

/*!
 * Return the name of the font.
 */
QString Format::fontName() const
{
	return stringProperty(FormatPrivate::P_Font_Name, QStringLiteral("Calibri"));
}

/*!
 * Set the name of the font to \a name.
 */
void Format::setFontName(const QString &name)
{
	setProperty(FormatPrivate::P_Font_Name, name, QStringLiteral("Calibri"));
}

/*!
 * Returns a QFont object based on font data contained in the format.
 */
QFont Format::font() const
{
   QFont font;
   font.setFamily(fontName());
   if (fontSize() > 0)
	   font.setPointSize(fontSize());
   font.setBold(fontBold());
   font.setItalic(fontItalic());
   font.setUnderline(fontUnderline()!=FontUnderlineNone);
   font.setStrikeOut(fontStrikeOut());
   return font;
}

/*!
 * Set the format properties from the given \a font.
 */
void Format::setFont(const QFont &font)
{
	setFontName(font.family());
	if (font.pointSize() > 0)
		setFontSize(font.pointSize());
	setFontBold(font.bold());
	setFontItalic(font.italic());
	setFontUnderline(font.underline() ? FontUnderlineSingle : FontUnderlineNone);
	setFontStrikeOut(font.strikeOut());
}

/*!
 * \internal
 * When the format has font data, when need to assign a valid index for it.
 * The index value is depend on the order <fonts > in styles.xml
 */
bool Format::fontIndexValid() const
{
	if (!hasFontData())
		return false;
	return d->font_index_valid;
}

/*!
 * \internal
 */
int Format::fontIndex() const
{
	if (fontIndexValid())
		return d->font_index;

	return 0;
}

/*!
 * \internal
 */
void Format::setFontIndex(int index)
{
	d->font_index = index;
	d->font_index_valid = true;
}

/*!
 * \internal
 */
QByteArray Format::fontKey() const
{
	if (isEmpty())
		return QByteArray();

	if (d->font_dirty) {
		QByteArray key;
		QDataStream stream(&key, QIODevice::WriteOnly);
		for (int i=FormatPrivate::P_Font_STARTID; i<FormatPrivate::P_Font_ENDID; ++i) {
            auto it = d->properties.constFind(i);
            if (it != d->properties.constEnd())
                stream << i << it.value();
		};

		const_cast<Format*>(this)->d->font_key = key;
		const_cast<Format*>(this)->d->font_dirty = false;
	}

	return d->font_key;
}

/*!
	\internal
	Return true if the format has font format, otherwise return false.
 */
bool Format::hasFontData() const
{
	if (!d)
		return false;

	for (int i=FormatPrivate::P_Font_STARTID; i<FormatPrivate::P_Font_ENDID; ++i) {
		if (hasProperty(i))
			return true;
	}
	return false;
}

/*!
 * Return the horizontal alignment.
 */
Format::HorizontalAlignment Format::horizontalAlignment() const
{
	return static_cast<Format::HorizontalAlignment>(intProperty(FormatPrivate::P_Alignment_AlignH, AlignHGeneral));
}

/*!
 * Set the horizontal alignment with the given \a align.
 */
void Format::setHorizontalAlignment(HorizontalAlignment align)
{
	if (hasProperty(FormatPrivate::P_Alignment_Indent)
			&&(align != AlignHGeneral && align != AlignLeft && align != AlignRight && align != AlignHDistributed)) {
		clearProperty(FormatPrivate::P_Alignment_Indent);
	}

	if (hasProperty(FormatPrivate::P_Alignment_ShinkToFit)
			&& (align == AlignHFill || align == AlignHJustify || align == AlignHDistributed)) {
		clearProperty(FormatPrivate::P_Alignment_ShinkToFit);
	}

	setProperty(FormatPrivate::P_Alignment_AlignH, align, AlignHGeneral);
}

/*!
 * Return the vertical alignment.
 */
Format::VerticalAlignment Format::verticalAlignment() const
{
	return static_cast<Format::VerticalAlignment>(intProperty(FormatPrivate::P_Alignment_AlignV, AlignBottom));
}

/*!
 * Set the vertical alignment with the given \a align.
 */
void Format::setVerticalAlignment(VerticalAlignment align)
{
	setProperty(FormatPrivate::P_Alignment_AlignV, align, AlignBottom);
}

/*!
 * Return whether the cell text is wrapped.
 */
bool Format::textWrap() const
{
	return boolProperty(FormatPrivate::P_Alignment_Wrap);
}

/*!
 * Enable the text wrap if \a wrap is true.
 */
void Format::setTextWrap(bool wrap)
{
	if (wrap && hasProperty(FormatPrivate::P_Alignment_ShinkToFit))
		clearProperty(FormatPrivate::P_Alignment_ShinkToFit);

	setProperty(FormatPrivate::P_Alignment_Wrap, wrap, false);
}

/*!
 * Return the text rotation.
 */
int Format::rotation() const
{
	return intProperty(FormatPrivate::P_Alignment_Rotation);
}

/*!
 * Set the text roation with the given \a rotation. Must be in the range [0, 180] or 255.
 */
void Format::setRotation(int rotation)
{
	setProperty(FormatPrivate::P_Alignment_Rotation, rotation, 0);
}

/*!
 * Return the text indentation level.
 */
int Format::indent() const
{
	return intProperty(FormatPrivate::P_Alignment_Indent);
}

/*!
 * Set the text indentation level with the given \a indent. Must be less than or equal to 15.
 */
void Format::setIndent(int indent)
{
	if (indent && hasProperty(FormatPrivate::P_Alignment_AlignH)) {
		HorizontalAlignment hl = horizontalAlignment();

		if (hl != AlignHGeneral && hl != AlignLeft && hl!= AlignRight && hl!= AlignHJustify) {
			setHorizontalAlignment(AlignLeft);
		}
	}

	setProperty(FormatPrivate::P_Alignment_Indent, indent, 0);
}

/*!
 * Return whether the cell is shrink to fit.
 */
bool Format::shrinkToFit() const
{
	return boolProperty(FormatPrivate::P_Alignment_ShinkToFit);
}

/*!
 * Turn on/off shrink to fit base on \a shink.
 */
void Format::setShrinkToFit(bool shink)
{
	if (shink && hasProperty(FormatPrivate::P_Alignment_Wrap))
		clearProperty(FormatPrivate::P_Alignment_Wrap);

	if (shink && hasProperty(FormatPrivate::P_Alignment_AlignH)) {
		HorizontalAlignment hl = horizontalAlignment();
		if (hl == AlignHFill || hl == AlignHJustify || hl == AlignHDistributed)
			setHorizontalAlignment(AlignLeft);
	}

	setProperty(FormatPrivate::P_Alignment_ShinkToFit, shink, false);
}

/*!
 * \internal
 */
bool Format::hasAlignmentData() const
{
	if (!d)
		return false;

	for (int i=FormatPrivate::P_Alignment_STARTID; i<FormatPrivate::P_Alignment_ENDID; ++i) {
		if (hasProperty(i))
			return true;
	}
	return false;
}

/*!
 * Set the border style with the given \a style.
 */
void Format::setBorderStyle(BorderStyle style)
{
	setLeftBorderStyle(style);
	setRightBorderStyle(style);
	setBottomBorderStyle(style);
	setTopBorderStyle(style);
}

/*!
 * Sets the border color with the given \a color.
 */
void Format::setBorderColor(const QColor &color)
{
	setLeftBorderColor(color);
	setRightBorderColor(color);
	setTopBorderColor(color);
	setBottomBorderColor(color);
}

/*!
 * Returns the left border style
 */
Format::BorderStyle Format::leftBorderStyle() const
{
	return static_cast<BorderStyle>(intProperty(FormatPrivate::P_Border_LeftStyle));
}

/*!
 * Sets the left border style to \a style
 */
void Format::setLeftBorderStyle(BorderStyle style)
{
	setProperty(FormatPrivate::P_Border_LeftStyle, style, BorderNone);
}

/*!
 * Returns the left border color
 */
QColor Format::leftBorderColor() const
{
	return colorProperty(FormatPrivate::P_Border_LeftColor);
}

/*!
	Sets the left border color to the given \a color
*/
void Format::setLeftBorderColor(const QColor &color)
{
	setProperty(FormatPrivate::P_Border_LeftColor, XlsxColor(color), XlsxColor());
}

/*!
	Returns the right border style.
*/
Format::BorderStyle Format::rightBorderStyle() const
{
	return static_cast<BorderStyle>(intProperty(FormatPrivate::P_Border_RightStyle));
}

/*!
	Sets the right border style to the given \a style.
*/
void Format::setRightBorderStyle(BorderStyle style)
{
	setProperty(FormatPrivate::P_Border_RightStyle, style, BorderNone);
}

/*!
	Returns the right border color.
*/
QColor Format::rightBorderColor() const
{
	return colorProperty(FormatPrivate::P_Border_RightColor);
}

/*!
	Sets the right border color to the given \a color
*/
void Format::setRightBorderColor(const QColor &color)
{
	setProperty(FormatPrivate::P_Border_RightColor, XlsxColor(color), XlsxColor());
}

/*!
	Returns the top border style.
*/
Format::BorderStyle Format::topBorderStyle() const
{
	return static_cast<BorderStyle>(intProperty(FormatPrivate::P_Border_TopStyle));
}

/*!
	Sets the top border style to the given \a style.
*/
void Format::setTopBorderStyle(BorderStyle style)
{
	setProperty(FormatPrivate::P_Border_TopStyle, style, BorderNone);
}

/*!
	Returns the top border color.
*/
QColor Format::topBorderColor() const
{
	return colorProperty(FormatPrivate::P_Border_TopColor);
}

/*!
	Sets the top border color to the given \a color.
*/
void Format::setTopBorderColor(const QColor &color)
{
	setProperty(FormatPrivate::P_Border_TopColor, XlsxColor(color), XlsxColor());
}

/*!
	Returns the bottom border style.
*/
Format::BorderStyle Format::bottomBorderStyle() const
{
	return static_cast<BorderStyle>(intProperty(FormatPrivate::P_Border_BottomStyle));
}

/*!
	Sets the bottom border style to the given \a style.
*/
void Format::setBottomBorderStyle(BorderStyle style)
{
	setProperty(FormatPrivate::P_Border_BottomStyle, style, BorderNone);
}

/*!
	Returns the bottom border color.
*/
QColor Format::bottomBorderColor() const
{
	return colorProperty(FormatPrivate::P_Border_BottomColor);
}

/*!
	Sets the bottom border color to the given \a color.
*/
void Format::setBottomBorderColor(const QColor &color)
{
	setProperty(FormatPrivate::P_Border_BottomColor, XlsxColor(color), XlsxColor());
}

/*!
	Return the diagonla border style.
*/
Format::BorderStyle Format::diagonalBorderStyle() const
{
	return static_cast<BorderStyle>(intProperty(FormatPrivate::P_Border_DiagonalStyle));
}

/*!
	Sets the diagonal border style to the given \a style.
*/
void Format::setDiagonalBorderStyle(BorderStyle style)
{
	setProperty(FormatPrivate::P_Border_DiagonalStyle, style, BorderNone);
}

/*!
	Returns the diagonal border type.
*/
Format::DiagonalBorderType Format::diagonalBorderType() const
{
	return static_cast<DiagonalBorderType>(intProperty(FormatPrivate::P_Border_DiagonalType));
}

/*!
	Sets the diagonal border type to the given \a style
*/
void Format::setDiagonalBorderType(DiagonalBorderType style)
{
	setProperty(FormatPrivate::P_Border_DiagonalType, style, DiagonalBorderNone);
}

/*!
	Returns the diagonal border color.
*/
QColor Format::diagonalBorderColor() const
{
	return colorProperty(FormatPrivate::P_Border_DiagonalColor);
}

/*!
	Sets the diagonal border color to the given \a color
*/
void Format::setDiagonalBorderColor(const QColor &color)
{
	setProperty(FormatPrivate::P_Border_DiagonalColor, XlsxColor(color), XlsxColor());
}

/*!
	\internal
	Returns whether this format has been set valid border index.
*/
bool Format::borderIndexValid() const
{
	if (!hasBorderData())
		return false;
	return d->border_index_valid;
}

/*!
	\internal
	Returns the border index.
*/
int Format::borderIndex() const
{
	if (borderIndexValid())
		return d->border_index;
	return 0;
}

/*!
 * \internal
 */
void Format::setBorderIndex(int index)
{
	d->border_index = index;
	d->border_index_valid = true;
}

/*! \internal
 */
QByteArray Format::borderKey() const
{
	if (isEmpty())
		return QByteArray();

	if (d->border_dirty) {
		QByteArray key;
		QDataStream stream(&key, QIODevice::WriteOnly);
		for (int i=FormatPrivate::P_Border_STARTID; i<FormatPrivate::P_Border_ENDID; ++i) {
            auto it = d->properties.constFind(i);
            if (it != d->properties.constEnd())
                stream << i << it.value();
		};

		const_cast<Format*>(this)->d->border_key = key;
		const_cast<Format*>(this)->d->border_dirty = false;
	}

	return d->border_key;
}

/*!
	\internal
	Return true if the format has border format, otherwise return false.
 */
bool Format::hasBorderData() const
{
	if (!d)
		return false;

	for (int i=FormatPrivate::P_Border_STARTID; i<FormatPrivate::P_Border_ENDID; ++i) {
		if (hasProperty(i))
			return true;
	}
	return false;
}

/*!
	Return the fill pattern.
*/
Format::FillPattern Format::fillPattern() const
{
	return static_cast<FillPattern>(intProperty(FormatPrivate::P_Fill_Pattern, PatternNone));
}

/*!
	Sets the fill pattern to the given \a pattern.
*/
void Format::setFillPattern(FillPattern pattern)
{
	setProperty(FormatPrivate::P_Fill_Pattern, pattern, PatternNone);
}

/*!
	Returns the foreground color of the pattern.
*/
QColor Format::patternForegroundColor() const
{
	return colorProperty(FormatPrivate::P_Fill_FgColor);
}

/*!
	Sets the foreground color of the pattern with the given \a color.
*/
void Format::setPatternForegroundColor(const QColor &color)
{
	if (color.isValid() && !hasProperty(FormatPrivate::P_Fill_Pattern))
		setFillPattern(PatternSolid);
	setProperty(FormatPrivate::P_Fill_FgColor, XlsxColor(color), XlsxColor());
}

/*!
	Returns the background color of the pattern.
*/
QColor Format::patternBackgroundColor() const
{
	return colorProperty(FormatPrivate::P_Fill_BgColor);
}

/*!
	Sets the background color of the pattern with the given \a color.
*/
void Format::setPatternBackgroundColor(const QColor &color)
{
	if (color.isValid() && !hasProperty(FormatPrivate::P_Fill_Pattern))
		setFillPattern(PatternSolid);
	setProperty(FormatPrivate::P_Fill_BgColor, XlsxColor(color), XlsxColor());
}

/*!
 * \internal
 */
bool Format::fillIndexValid() const
{
	if (!hasFillData())
		return false;
	return d->fill_index_valid;
}

/*!
 * \internal
 */
int Format::fillIndex() const
{
	if (fillIndexValid())
		return d->fill_index;
	return 0;
}

/*!
 * \internal
 */
void Format::setFillIndex(int index)
{
	d->fill_index = index;
	d->fill_index_valid = true;
}

/*!
 * \internal
 */
QByteArray Format::fillKey() const
{
	if (isEmpty())
		return QByteArray();

	if (d->fill_dirty) {
		QByteArray key;
		QDataStream stream(&key, QIODevice::WriteOnly);
		for (int i=FormatPrivate::P_Fill_STARTID; i<FormatPrivate::P_Fill_ENDID; ++i) {
            auto it = d->properties.constFind(i);
            if (it != d->properties.constEnd())
                stream << i << it.value();
		};

		const_cast<Format*>(this)->d->fill_key = key;
		const_cast<Format*>(this)->d->fill_dirty = false;
	}

	return d->fill_key;
}

/*!
	\internal
	Return true if the format has fill format, otherwise return false.
 */
bool Format::hasFillData() const
{
	if (!d)
		return false;

	for (int i=FormatPrivate::P_Fill_STARTID; i<FormatPrivate::P_Fill_ENDID; ++i) {
		if (hasProperty(i))
			return true;
	}
	return false;
}

/*!
	Returns whether the hidden protection property is set to true.
*/
bool Format::hidden() const
{
	return boolProperty(FormatPrivate::P_Protection_Hidden);
}

/*!
	Sets the hidden protection property with the given \a hidden.
*/
void Format::setHidden(bool hidden)
{
	setProperty(FormatPrivate::P_Protection_Hidden, hidden);
}

/*!
	Returns whether the locked protection property is set to true.
*/
bool Format::locked() const
{
	return boolProperty(FormatPrivate::P_Protection_Locked);
}

/*!
	Sets the locked protection property with the given \a locked.
*/
void Format::setLocked(bool locked)
{
	setProperty(FormatPrivate::P_Protection_Locked, locked);
}

/*!
	\internal
	Return true if the format has protection data, otherwise return false.
 */
bool Format::hasProtectionData() const
{
	if (!d)
		return false;

	if (hasProperty(FormatPrivate::P_Protection_Hidden)
			|| hasProperty(FormatPrivate::P_Protection_Locked)) {
		return true;
	}
	return false;
}

/*!
	Merges the current format with the properties described by format \a modifier.
 */
void Format::mergeFormat(const Format &modifier)
{
	if (!modifier.isValid())
		return;

	if (!isValid()) {
		d = modifier.d;
		return;
	}

	QMapIterator<int, QVariant> it(modifier.d->properties);
	while(it.hasNext()) {
		it.next();
		setProperty(it.key(), it.value());
	}
}

/*!
	Returns true if the format is valid; otherwise returns false.
 */
bool Format::isValid() const
{
	if (d)
		return true;
	return false;
}

/*!
	Returns true if the format is empty; otherwise returns false.
 */
bool Format::isEmpty() const
{
	if (!d)
		return true;
	return d->properties.isEmpty();
}

/*!
 * \internal
 */
QByteArray Format::formatKey() const
{
	if (isEmpty())
		return QByteArray();

	if (d->dirty) {
		QByteArray key;
		QDataStream stream(&key, QIODevice::WriteOnly);

		QMapIterator<int, QVariant> i(d->properties);
		while (i.hasNext()) {
			i.next();
			stream<<i.key()<<i.value();
		}

		d->formatKey = key;
		d->dirty = false;
	}

	return d->formatKey;
}

/*!
 * \internal
 *  Called by QXlsx::Styles or some unittests.
 */
void Format::setXfIndex(int index)
{
	if (!d)
		d = new FormatPrivate;
	d->xf_index = index;
	d->xf_indexValid = true;
}

/*!
 * \internal
 */
int Format::xfIndex() const
{
	if (!d)
		return -1;
	return d->xf_index;
}

/*!
 * \internal
 */
bool Format::xfIndexValid() const
{
	if (!d)
		return false;
	return d->xf_indexValid;
}

/*!
 * \internal
 *  Called by QXlsx::Styles or some unittests.
 */
void Format::setDxfIndex(int index)
{
	if (!d)
		d = new FormatPrivate;
	d->dxf_index = index;
	d->dxf_indexValid = true;
}

/*!
 * \internal
 * Returns the index in the styles dxfs.
 */
int Format::dxfIndex() const
{
	if (!d)
		return -1;
	return d->dxf_index;
}

/*!
 * \internal
 * Returns whether the dxf index is valid or not.
 */
bool Format::dxfIndexValid() const
{
	if (!d)
		return false;
	return d->dxf_indexValid;
}

/*!
	Returns ture if the \a format is equal to this format.
*/
bool Format::operator ==(const Format &format) const
{
	return this->formatKey() == format.formatKey();
}

/*!
	Returns ture if the \a format is not equal to this format.
*/
bool Format::operator !=(const Format &format) const
{
	return this->formatKey() != format.formatKey();
}

int Format::theme() const
{
	return d->theme;
}

/*!
 * \internal
 */
QVariant Format::property(int propertyId, const QVariant &defaultValue) const
{
    if (d) {
        auto it = d->properties.constFind(propertyId);
        if (it != d->properties.constEnd())
            return it.value();
    }
	return defaultValue;
}

/*!
 * \internal
 */
void Format::setProperty(int propertyId, const QVariant &value, const QVariant &clearValue, bool detach)
{
	if (!d)
		d = new FormatPrivate;

    if (value != clearValue)
    {
        auto it = d->properties.constFind(propertyId);
        if (it != d->properties.constEnd() && it.value() == value)
			return;

		if (detach)
			d.detach();

		d->properties[propertyId] = value;
    }
    else
    {
		if (!d->properties.contains(propertyId))
			return;

		if (detach)
			d.detach();

		d->properties.remove(propertyId);
	}

	d->dirty = true;
	d->xf_indexValid = false;
	d->dxf_indexValid = false;

    if (propertyId >= FormatPrivate::P_Font_STARTID && propertyId < FormatPrivate::P_Font_ENDID)
    {
		d->font_dirty = true;
		d->font_index_valid = false;
    }
    else if (propertyId >= FormatPrivate::P_Border_STARTID && propertyId < FormatPrivate::P_Border_ENDID)
    {
		d->border_dirty = true;
		d->border_index_valid = false;
    }
    else if (propertyId >= FormatPrivate::P_Fill_STARTID && propertyId < FormatPrivate::P_Fill_ENDID)
    {
		d->fill_dirty = true;
		d->fill_index_valid = false;
	}
}

/*!
 * \internal
 */
void Format::clearProperty(int propertyId)
{
	setProperty(propertyId, QVariant());
}

/*!
 * \internal
 */
bool Format::hasProperty(int propertyId) const
{
	if (!d)
		return false;
	return d->properties.contains(propertyId);
}

/*!
 * \internal
 */
bool Format::boolProperty(int propertyId, bool defaultValue) const
{
	if (!hasProperty(propertyId))
		return defaultValue;

	const QVariant prop = d->properties[propertyId];
	if (prop.userType() != QMetaType::Bool)
		return defaultValue;
	return prop.toBool();
}

/*!
 * \internal
 */
int Format::intProperty(int propertyId, int defaultValue) const
{
	if (!hasProperty(propertyId))
		return defaultValue;

	const QVariant prop = d->properties[propertyId];
	if (prop.userType() != QMetaType::Int)
		return defaultValue;
	return prop.toInt();
}

/*!
 * \internal
 */
double Format::doubleProperty(int propertyId, double defaultValue) const
{
	if (!hasProperty(propertyId))
		return defaultValue;

	const QVariant prop = d->properties[propertyId];
	if (prop.userType() != QMetaType::Double && prop.userType() != QMetaType::Float)
		return defaultValue;
	return prop.toDouble();
}

/*!
 * \internal
 */
QString Format::stringProperty(int propertyId, const QString &defaultValue) const
{
	if (!hasProperty(propertyId))
		return defaultValue;

	const QVariant prop = d->properties[propertyId];
	if (prop.userType() != QMetaType::QString)
		return defaultValue;
	return prop.toString();
}

/*!
 * \internal
 */
QColor Format::colorProperty(int propertyId, const QColor &defaultValue) const
{
	if (!hasProperty(propertyId))
		return defaultValue;

	const QVariant prop = d->properties[propertyId];
	if (prop.userType() != qMetaTypeId<XlsxColor>())
		return defaultValue;
	return qvariant_cast<XlsxColor>(prop).rgbColor();
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const Format &f)
{
	dbg.nospace() << "QXlsx::Format(" << f.d->properties << ")";
	return dbg.space();
}
#endif

QT_END_NAMESPACE_XLSX
