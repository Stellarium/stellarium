/*
 * Copyright (C) 2020 Georg Zotti
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#ifndef CALENDARS_HPP
#define CALENDARS_HPP

#include <QFont>
#include <QMap>

#include "StelModule.hpp"
#include "StelScriptMgr.hpp"


#include "Calendar.hpp"
#include "../plugins/Calendars/src/gui/CalendarsInfoPanel.hpp"

class CalendarsDialog;
class StelButton;

/*! @defgroup calendars Calendars Plug-in
@{
The Calendars plugin provides an interface to various calendars

The primary source of this plugin is the book "Calendrical Calculations: The Ultimate Edition"
by Edward M. Reingold and Nachum Dershowitz (2018). It contains algorithmic descriptions of dozens of calendars and auxiliary functions,
most of which should make their way into this plugin.

This book describes data conversion from and to calendars, using not the commonly used Julian Day number, but an intermediate
number called <em>Rata Die</em> (R.D.; easily remembered by the authors' names), days counted from midnight of (proleptic) 1.1. of year 1 AD (Gregorian).

For the user, a simple selection GUI allows choosing which calendars should be displayed in the lower-right screen corner.
Some more GUI tabs allow interaction with selected calendars.

A potentially great feature for owners of the book is that most functions from the book are available as scripting functions for the respective calendars.
Just call objects by their classnames.

Examples:

core.output(JulianCalendar.isLeap(1700));
core.output(GregorianCalendar.isLeap(1700));
rd=GregorianCalendar.fixedFromGregorian{[2021, 3, 14]);

Take care that some data arguments are internally stored as QVector<int>, and translated to Arrays in ECMAscript.
The various calendars may have array lengths of elements, which are not always checked.
When a StelLocation argument is used in the internal function, a scripting function is available which allows specifying
a location name in format "city, region". This also works with user-specified locations.
Time zones only work correctly when specified (in the location database) as full specification like "Europe/Madrid",
or in a generic offset spelling like "UTC+04:00" (but not "UT+4"). For use of "calendar locations" that have been used before
timezones were introduced, we must work out longitude-dependent UTC offset that must be rounded to the nearest minute.

Occasionally, the time zone support will provide different results between Windows and other operating systems, esp. for historical data.
See Qt documentation on QtTimeZone.

@}
*/

//! @class Calendars
//! StelModule plugin which provides display and a scripting interface to a multitude of calendrical functions.
//! @author Georg Zotti
//! @ingroup calendars

class Calendars : public StelModule
{
	Q_OBJECT
	Q_PROPERTY(bool enabled		      READ isEnabled                WRITE enable            NOTIFY enabledChanged)

	Q_PROPERTY(bool flagTextColorOverride READ getFlagTextColorOverride WRITE setFlagTextColorOverride NOTIFY flagTextColorOverrideChanged)
	Q_PROPERTY(Vec3f textColor            READ getTextColor             WRITE setTextColor      NOTIFY textColorChanged)

	Q_PROPERTY(bool flagShowJulian        READ isJulianDisplayed        WRITE showJulian        NOTIFY showJulianChanged)
	Q_PROPERTY(bool flagShowRevisedJulian READ isRevisedJulianDisplayed WRITE showRevisedJulian NOTIFY showRevisedJulianChanged)
	Q_PROPERTY(bool flagShowGregorian     READ isGregorianDisplayed     WRITE showGregorian     NOTIFY showGregorianChanged)
	Q_PROPERTY(bool flagShowISO           READ isISODisplayed           WRITE showISO           NOTIFY showISOChanged)
	Q_PROPERTY(bool flagShowIcelandic     READ isIcelandicDisplayed     WRITE showIcelandic     NOTIFY showIcelandicChanged)
	Q_PROPERTY(bool flagShowRoman         READ isRomanDisplayed         WRITE showRoman         NOTIFY showRomanChanged)
	Q_PROPERTY(bool flagShowOlympic       READ isOlympicDisplayed       WRITE showOlympic       NOTIFY showOlympicChanged)
	Q_PROPERTY(bool flagShowEgyptian      READ isEgyptianDisplayed      WRITE showEgyptian      NOTIFY showEgyptianChanged)
	Q_PROPERTY(bool flagShowArmenian      READ isArmenianDisplayed      WRITE showArmenian      NOTIFY showArmenianChanged)
	Q_PROPERTY(bool flagShowZoroastrian   READ isZoroastrianDisplayed   WRITE showZoroastrian   NOTIFY showZoroastrianChanged)
	Q_PROPERTY(bool flagShowCoptic        READ isCopticDisplayed        WRITE showCoptic        NOTIFY showCopticChanged)
	Q_PROPERTY(bool flagShowEthiopic      READ isEthiopicDisplayed      WRITE showEthiopic      NOTIFY showEthiopicChanged)
	Q_PROPERTY(bool flagShowChinese       READ isChineseDisplayed       WRITE showChinese       NOTIFY showChineseChanged)
	Q_PROPERTY(bool flagShowJapanese      READ isJapaneseDisplayed      WRITE showJapanese      NOTIFY showJapaneseChanged)
	Q_PROPERTY(bool flagShowKorean        READ isKoreanDisplayed        WRITE showKorean        NOTIFY showKoreanChanged)
	Q_PROPERTY(bool flagShowVietnamese    READ isVietnameseDisplayed    WRITE showVietnamese    NOTIFY showVietnameseChanged)
	Q_PROPERTY(bool flagShowIslamic       READ isIslamicDisplayed       WRITE showIslamic       NOTIFY showIslamicChanged)
	Q_PROPERTY(bool flagShowHebrew        READ isHebrewDisplayed        WRITE showHebrew        NOTIFY showHebrewChanged)
	Q_PROPERTY(bool flagShowOldHinduSolar READ isOldHinduSolarDisplayed WRITE showOldHinduSolar NOTIFY showOldHinduSolarChanged)
	Q_PROPERTY(bool flagShowOldHinduLunar READ isOldHinduLunarDisplayed WRITE showOldHinduLunar NOTIFY showOldHinduLunarChanged)
	Q_PROPERTY(bool flagShowNewHinduSolar READ isNewHinduSolarDisplayed WRITE showNewHinduSolar NOTIFY showNewHinduSolarChanged)
	Q_PROPERTY(bool flagShowNewHinduLunar READ isNewHinduLunarDisplayed WRITE showNewHinduLunar NOTIFY showNewHinduLunarChanged)
	Q_PROPERTY(bool flagShowAstroHinduSolar READ isAstroHinduSolarDisplayed WRITE showAstroHinduSolar NOTIFY showAstroHinduSolarChanged)
	Q_PROPERTY(bool flagShowAstroHinduLunar READ isAstroHinduLunarDisplayed WRITE showAstroHinduLunar NOTIFY showAstroHinduLunarChanged)
	Q_PROPERTY(bool flagShowMayaLongCount READ isMayaLongCountDisplayed WRITE showMayaLongCount NOTIFY showMayaLongCountChanged)
	Q_PROPERTY(bool flagShowMayaHaab      READ isMayaHaabDisplayed      WRITE showMayaHaab      NOTIFY showMayaHaabChanged)
	Q_PROPERTY(bool flagShowMayaTzolkin   READ isMayaTzolkinDisplayed   WRITE showMayaTzolkin   NOTIFY showMayaTzolkinChanged)
	Q_PROPERTY(bool flagShowAztecXihuitl  READ isAztecXihuitlDisplayed  WRITE showAztecXihuitl  NOTIFY showAztecXihuitlChanged)
	Q_PROPERTY(bool flagShowAztecTonalpohualli READ isAztecTonalpohualliDisplayed WRITE showAztecTonalpohualli NOTIFY showAztecTonalpohualliChanged)
	Q_PROPERTY(bool flagShowBalinese      READ isBalineseDisplayed      WRITE showBalinese      NOTIFY showBalineseChanged)
	Q_PROPERTY(bool flagShowFrenchAstronomical READ isFrenchAstronomicalDisplayed WRITE showFrenchAstronomical NOTIFY showFrenchAstronomicalChanged)
	Q_PROPERTY(bool flagShowFrenchArithmetic   READ isFrenchArithmeticDisplayed   WRITE showFrenchArithmetic   NOTIFY showFrenchArithmeticChanged)
	Q_PROPERTY(bool flagShowPersianArithmetic   READ isPersianArithmeticDisplayed   WRITE showPersianArithmetic   NOTIFY showPersianArithmeticChanged)
	Q_PROPERTY(bool flagShowPersianAstronomical READ isPersianAstronomicalDisplayed WRITE showPersianAstronomical NOTIFY showPersianAstronomicalChanged)
	Q_PROPERTY(bool flagShowBahaiArithmetic     READ isBahaiArithmeticDisplayed   WRITE showBahaiArithmetic   NOTIFY showBahaiArithmeticChanged)
	Q_PROPERTY(bool flagShowBahaiAstronomical   READ isBahaiAstronomicalDisplayed WRITE showBahaiAstronomical NOTIFY showBahaiAstronomicalChanged)
	Q_PROPERTY(bool flagShowTibetan      READ isTibetanDisplayed        WRITE showTibetan       NOTIFY showTibetanChanged)

public:
	Calendars();
	~Calendars() override;

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	void init() override;
	//! Set all calendars to the Core's JD.
	void update(double) override;
	//! if enabled, provide a table of calendars on screen.
	void draw(StelCore* core) override;
	double getCallOrder(StelModuleActionName actionName) const override;
	bool configureGui(bool show=true) override;

	//! Restore the plug-in's settings to the default state.
	//! Replace the plug-in's settings in Stellarium's configuration file
	//! with the default values and re-load them.
	//! Uses internally loadSettings().
	void restoreDefaultSettings();

	//! Load the plug-in's settings from the configuration file.
	//! Settings are kept in the "Calendars" section in Stellarium's
	//! configuration file. If no such section exists, it will load default
	//! values.
	//! @see restoreDefaultSettings()
	void loadSettings();

	//! Get a pointer to the respective Calendar. Returns nullptr if not found.
	//! Valid names: Julian, RevisedJulian, Gregorian, ISO, Icelandic, Roman, Olympic, Egyptian,
	//! Armenian, Zoroastrian, Coptic, Ethiopic, Islamic, Hebrew,
	//! OldHinduSolar, OldHinduLunar, NewHinduSolar, NewHinduLunar, Balinese, Tibetan,
	//! BahaiArithmetic, BahaiAstronomical,
	//! MayaLongCount, MayaHaab, MayaTzolkin, AztecXihuitl, AztecTonalpohualli
	//! TODO: ADD HERE: Chinese,  ...
	Calendar* getCal(QString name);

	#ifdef ENABLE_SCRIPTING
	//! to be called after program startup, when StelScriptMgr has been set up.
	void makeCalendarsScriptable(StelScriptMgr *ssm);
	#endif

signals:
	//void jdChanged(double jd);

	void enabledChanged(bool b);
	void flagTextColorOverrideChanged(bool b);
	void textColorChanged(Vec3f &color);
	void showJulianChanged(bool b);
	void showRevisedJulianChanged(bool b);
	void showGregorianChanged(bool b);
	void showISOChanged(bool b);
	void showIcelandicChanged(bool b);
	void showRomanChanged(bool b);
	void showOlympicChanged(bool b);
	void showEgyptianChanged(bool b);
	void showArmenianChanged(bool b);
	void showZoroastrianChanged(bool b);
	void showCopticChanged(bool b);
	void showEthiopicChanged(bool b);
	void showChineseChanged(bool b);
	void showJapaneseChanged(bool b);
	void showKoreanChanged(bool b);
	void showVietnameseChanged(bool b);
	void showIslamicChanged(bool b);
	void showHebrewChanged(bool b);
	void showOldHinduSolarChanged(bool b);
	void showOldHinduLunarChanged(bool b);
	void showNewHinduSolarChanged(bool b);
	void showNewHinduLunarChanged(bool b);
	void showAstroHinduSolarChanged(bool b);
	void showAstroHinduLunarChanged(bool b);
	void showMayaLongCountChanged(bool b);
	void showMayaHaabChanged(bool b);
	void showMayaTzolkinChanged(bool b);
	void showAztecXihuitlChanged(bool b);
	void showAztecTonalpohualliChanged(bool b);
	void showBalineseChanged(bool b);
	void showFrenchAstronomicalChanged(bool b);
	void showFrenchArithmeticChanged(bool b);
	void showPersianArithmeticChanged(bool b);
	void showPersianAstronomicalChanged(bool b);
	void showBahaiArithmeticChanged(bool b);
	void showBahaiAstronomicalChanged(bool b);
	void showTibetanChanged(bool b);

public slots:
	// Setters/getters
	//! is display of calendars overlay active?
	bool isEnabled() const;
	bool getFlagTextColorOverride() const;
	void setFlagTextColorOverride(bool b);
	//! Get the current color of the text (when override flag is active).
	Vec3f getTextColor() const;
	void setTextColor(const Vec3f& newColor);

	//! enable display of calendars overlay
	void enable(bool b);
	bool isJulianDisplayed() const;		//!< display Julian Calendar?
	void showJulian(bool b);		//!< activate display of Julian Calendar
	bool isRevisedJulianDisplayed() const;	//!< display Revised Julian Calendar?
	void showRevisedJulian(bool b);		//!< activate display of Revised Julian Calendar
	bool isGregorianDisplayed() const;	//!< display Gregorian Calendar?
	void showGregorian(bool b);		//!< activate display of Gregorian Calendar
	bool isISODisplayed() const;		//!< display ISO Calendar?
	void showISO(bool b);			//!< activate display of ISO Calendar
	bool isIcelandicDisplayed() const;	//!< display Icelandic Calendar?
	void showIcelandic(bool b);		//!< activate display of Icelandic Calendar
	bool isRomanDisplayed() const;		//!< display Roman Calendar?
	void showRoman(bool b);			//!< activate display of Roman Calendar
	bool isOlympicDisplayed() const;	//!< display Olympic Calendar?
	void showOlympic(bool b);		//!< activate display of Olympic Calendar
	bool isEgyptianDisplayed() const;	//!< display Egyptian Calendar?
	void showEgyptian(bool b);		//!< activate display of Egyptian Calendar
	bool isArmenianDisplayed() const;	//!< display Armenian Calendar?
	void showArmenian(bool b);		//!< activate display of Armenian Calendar
	bool isZoroastrianDisplayed() const;	//!< display Zoroastrian Calendar?
	void showZoroastrian(bool b);		//!< activate display of Zoroastrian Calendar
	bool isCopticDisplayed() const;		//!< display Coptic Calendar?
	void showCoptic(bool b);		//!< activate display of Coptic Calendar
	bool isEthiopicDisplayed() const;	//!< display Ethiopic Calendar?
	void showEthiopic(bool b);		//!< activate display of Ethiopic Calendar
	bool isChineseDisplayed() const;	//!< display Chinese Calendar?
	void showChinese(bool b);		//!< activate display of Chinese Calendar
	bool isJapaneseDisplayed() const;	//!< display Japanese Calendar?
	void showJapanese(bool b);		//!< activate display of Japanese Calendar
	bool isKoreanDisplayed() const;	//!< display Korean Calendar?
	void showKorean(bool b);		//!< activate display of Korean Calendar
	bool isVietnameseDisplayed() const;	//!< display Vietnamese Calendar?
	void showVietnamese(bool b);		//!< activate display of Vietnamese Calendar
	bool isIslamicDisplayed() const;	//!< display Islamic Calendar?
	void showIslamic(bool b);		//!< activate display of Islamic Calendar
	bool isHebrewDisplayed() const;		//!< display Hebrew Calendar?
	void showHebrew(bool b);		//!< activate display of Hebrew Calendar
	bool isOldHinduSolarDisplayed() const;	//!< display Old Hindu Solar?
	void showOldHinduSolar(bool b);		//!< activate display of Old Hindu Solar
	bool isOldHinduLunarDisplayed() const;	//!< display Old Hindu Lunar?
	void showOldHinduLunar(bool b);		//!< activate display of Old Hindu Lunar
	bool isNewHinduSolarDisplayed() const;	//!< display New Hindu Solar?
	void showNewHinduSolar(bool b);		//!< activate display of New Hindu Solar
	bool isNewHinduLunarDisplayed() const;	//!< display New Hindu Lunisolar?
	void showNewHinduLunar(bool b);		//!< activate display of New Hindu Lunisolar
	bool isAstroHinduSolarDisplayed() const;//!< display Astro Hindu Solar?
	void showAstroHinduSolar(bool b);	//!< activate display of Astro Hindu Solar
	bool isAstroHinduLunarDisplayed() const;//!< display Astro Hindu Lunisolar?
	void showAstroHinduLunar(bool b);	//!< activate display of Astro Hindu Lunisolar
	bool isMayaLongCountDisplayed() const;	//!< display Maya Long Count?
	void showMayaLongCount(bool b);		//!< activate display of Maya Long Count
	bool isMayaHaabDisplayed() const;	//!< display Maya Haab?
	void showMayaHaab(bool b);		//!< activate display of Maya Haab
	bool isMayaTzolkinDisplayed() const;	//!< display Maya Tzolkin?
	void showMayaTzolkin(bool b);		//!< activate display of Maya Tzolkin
	bool isAztecXihuitlDisplayed() const;	//!< display Aztec Xihuitl?
	void showAztecXihuitl(bool b);		//!< activate display of Aztec Xihuitl
	bool isAztecTonalpohualliDisplayed() const; //!< display Aztec Tonalpohualli?
	void showAztecTonalpohualli(bool b);	//!< activate display of Aztec Tonalpohualli
	bool isBalineseDisplayed() const;       //!< display Balinese Pawukon?
	void showBalinese(bool b);	        //!< activate display of Balinese Pawukon
	bool isFrenchAstronomicalDisplayed() const; //!< display French Astronomical?
	void showFrenchAstronomical(bool b);	//!< activate display of French Astronomical
	bool isFrenchArithmeticDisplayed() const; //!< display French Arithmetic?
	void showFrenchArithmetic(bool b);	//!< activate display of French Arithmetic
	bool isPersianArithmeticDisplayed() const; //!< display Persian Arithmetic?
	void showPersianArithmetic(bool b);	//!< activate display of Persian Arithmetic
	bool isPersianAstronomicalDisplayed() const; //!< display Persian Astronomical?
	void showPersianAstronomical(bool b);	//!< activate display of Persian Astronomical
	bool isBahaiArithmeticDisplayed() const;//!< display Bahai Arithmetic?
	void showBahaiArithmetic(bool b);	//!< activate display of Bahai Arithmetic
	bool isBahaiAstronomicalDisplayed() const; //!< display Bahai Astronomical?
	void showBahaiAstronomical(bool b);	//!< activate display of Bahai Astronomical
	bool isTibetanDisplayed() const;        //!< display Tibetan?
	void showTibetan(bool b);	        //!< activate display of Tibetan

private:
	// Font used for displaying text
	QFont font;
	CalendarsInfoPanel *infoPanel;
	CalendarsDialog* configDialog;
	StelButton* toolbarButton;
	QSettings* conf;

	// a QMap of pointers to calendars. The Names are identical to the respective Calendar subclass names, minus "Calendar".
	QMap<QString, Calendar*> calendars;

	// This is used to buffer JD for all known subcalendars.
	// Maybe we don't need it in the end...
	//double JD;

	// StelProperties:
	bool enabled;
	bool flagTextColorOverride;
	Vec3f textColor;

	bool flagShowJulian;
	bool flagShowRevisedJulian;
	bool flagShowGregorian;
	bool flagShowISO;
	bool flagShowIcelandic;
	bool flagShowRoman;
	bool flagShowOlympic;
	bool flagShowEgyptian;
	bool flagShowArmenian;
	bool flagShowZoroastrian;
	bool flagShowCoptic;
	bool flagShowEthiopic;
	bool flagShowChinese;
	bool flagShowJapanese;
	bool flagShowKorean;
	bool flagShowVietnamese;
	bool flagShowIslamic;
	bool flagShowHebrew;
	bool flagShowOldHinduSolar;
	bool flagShowOldHinduLunar;
	bool flagShowNewHinduSolar;
	bool flagShowNewHinduLunar;
	bool flagShowAstroHinduSolar;
	bool flagShowAstroHinduLunar;
	bool flagShowMayaLongCount;
	bool flagShowMayaHaab;
	bool flagShowMayaTzolkin;
	bool flagShowAztecXihuitl;
	bool flagShowAztecTonalpohualli;
	bool flagShowBalinese;
	bool flagShowFrenchAstronomical;
	bool flagShowFrenchArithmetic;
	bool flagShowPersianArithmetic;
	bool flagShowPersianAstronomical;
	bool flagShowBahaiArithmetic;
	bool flagShowBahaiAstronomical;
	bool flagShowTibetan;
};


#include <QObject>
#include "StelPluginInterface.hpp"

//! This class is used by Qt to manage a plug-in interface
class CalendarsStelPluginInterface : public QObject, public StelPluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID StelPluginInterface_iid)
	Q_INTERFACES(StelPluginInterface)
public:
	StelModule* getStelModule() const override;
	StelPluginInfo getPluginInfo() const override;
	//QObjectList getExtensionList() const override { return QObjectList(); }
};

#endif /* CALENDARS_HPP */
