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


#include "Calendar.hpp"
#include "CalendarsInfoPanel.hpp"

class CalendarsDialog;
class StelButton;

//! Calendars plugin provides an interface to various calendars
class Calendars : public StelModule
{
	Q_OBJECT
	Q_PROPERTY(bool enabled		      READ isEnabled                WRITE enable            NOTIFY enabledChanged)
	Q_PROPERTY(bool flagShowJulian        READ isJulianDisplayed        WRITE showJulian        NOTIFY showJulianChanged)
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
	Q_PROPERTY(bool flagShowIslamic       READ isIslamicDisplayed       WRITE showIslamic       NOTIFY showIslamicChanged)
	Q_PROPERTY(bool flagShowHebrew        READ isHebrewDisplayed        WRITE showHebrew        NOTIFY showHebrewChanged)
	Q_PROPERTY(bool flagShowOldHinduSolar READ isOldHinduSolarDisplayed WRITE showOldHinduSolar NOTIFY showOldHinduSolarChanged)
	Q_PROPERTY(bool flagShowOldHinduLunar READ isOldHinduLunarDisplayed WRITE showOldHinduLunar NOTIFY showOldHinduLunarChanged)
	Q_PROPERTY(bool flagShowMayaLongCount READ isMayaLongCountDisplayed WRITE showMayaLongCount NOTIFY showMayaLongCountChanged)
	Q_PROPERTY(bool flagShowMayaHaab      READ isMayaHaabDisplayed      WRITE showMayaHaab      NOTIFY showMayaHaabChanged)
	Q_PROPERTY(bool flagShowMayaTzolkin   READ isMayaTzolkinDisplayed   WRITE showMayaTzolkin   NOTIFY showMayaTzolkinChanged)
	Q_PROPERTY(bool flagShowAztecXihuitl  READ isAztecXihuitlDisplayed  WRITE showAztecXihuitl  NOTIFY showAztecXihuitlChanged)
	Q_PROPERTY(bool flagShowAztecTonalpohualli READ isAztecTonalpohualliDisplayed WRITE showAztecTonalpohualli NOTIFY showAztecTonalpohualliChanged)
	Q_PROPERTY(bool flagShowBalinese      READ isBalineseDisplayed      WRITE showBalinese      NOTIFY showBalineseChanged)
	Q_PROPERTY(bool flagShowFrenchArithmetic   READ isFrenchArithmeticDisplayed   WRITE showFrenchArithmetic   NOTIFY showFrenchArithmeticChanged)
	Q_PROPERTY(bool flagShowPersianArithmetic  READ isPersianArithmeticDisplayed  WRITE showPersianArithmetic  NOTIFY showPersianArithmeticChanged)

public:
	Calendars();
	virtual ~Calendars() Q_DECL_OVERRIDE;

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	virtual void init() Q_DECL_OVERRIDE;
	//! Set all calendars to the Core's JD.
	virtual void update(double) Q_DECL_OVERRIDE;
	//! if enabled, provide a table of calendars on screen.
	virtual void draw(StelCore* core) Q_DECL_OVERRIDE;
	virtual double getCallOrder(StelModuleActionName actionName) const Q_DECL_OVERRIDE;
	virtual bool configureGui(bool show=true) Q_DECL_OVERRIDE;

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

	//! Get a pointer to the respective Calendar. Returns Q_NULLPTR if not found.
	//! Valid names: Julian, Gregorian, ISO, Icelandic, Roman, Olympic, Egyptian,
	//! Armenian, Zoroastrian, Coptic, Ethiopic, Islamic, Hebrew,
	//! OldHinduSolar, OldHinduLunar, Balinese
	//! MayaLongCount, MayaHaab, MayaTzolkin, AztecXihuitl, AztecTonalpohualli
	//! TODO: ADD HERE: Chinese, NewHinduSolar, NewHinduLunar, Tibetan, ...
	Calendar* getCal(QString name);

signals:
	//void jdChanged(double jd);

	void enabledChanged(bool b);
	void showJulianChanged(bool b);
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
	void showIslamicChanged(bool b);
	void showHebrewChanged(bool b);
	void showOldHinduSolarChanged(bool b);
	void showOldHinduLunarChanged(bool b);
	void showMayaLongCountChanged(bool b);
	void showMayaHaabChanged(bool b);
	void showMayaTzolkinChanged(bool b);
	void showAztecXihuitlChanged(bool b);
	void showAztecTonalpohualliChanged(bool b);
	void showBalineseChanged(bool b);
	void showFrenchArithmeticChanged(bool b);
	void showPersianArithmeticChanged(bool b);

public slots:
	// Setters/getters
	//! is display of calendars overlay active?
	bool isEnabled() const;
	//! enable display of calendars overlay
	void enable(bool b);
	bool isJulianDisplayed() const;		//!< display Julian Calendar?
	void showJulian(bool b);		//!< activate display of Julian Calendar
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
	bool isIslamicDisplayed() const;	//!< display Islamic Calendar?
	void showIslamic(bool b);		//!< activate display of Islamic Calendar
	bool isHebrewDisplayed() const;		//!< display Hebrew Calendar?
	void showHebrew(bool b);		//!< activate display of Hebrew Calendar
	bool isOldHinduSolarDisplayed() const;	//!< display Old Hindu Solar?
	void showOldHinduSolar(bool b);		//!< activate display of Old Hindu Solar
	bool isOldHinduLunarDisplayed() const;	//!< display Old Hindu Lunar?
	void showOldHinduLunar(bool b);		//!< activate display of Old Hindu Lunar
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
	bool isFrenchArithmeticDisplayed() const; //!< display French Arithmetic?
	void showFrenchArithmetic(bool b);	  //!< activate display of French Arithmetic
	bool isPersianArithmeticDisplayed() const; //!< display Persian Arithmetic?
	void showPersianArithmetic(bool b);	  //!< activate display of Persian Arithmetic

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
	bool flagShowJulian;
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
	bool flagShowIslamic;
	bool flagShowHebrew;
	bool flagShowOldHinduSolar;
	bool flagShowOldHinduLunar;
	bool flagShowMayaLongCount;
	bool flagShowMayaHaab;
	bool flagShowMayaTzolkin;
	bool flagShowAztecXihuitl;
	bool flagShowAztecTonalpohualli;
	bool flagShowBalinese;
	bool flagShowFrenchArithmetic;
	bool flagShowPersianArithmetic;
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
	virtual StelModule* getStelModule() const Q_DECL_OVERRIDE;
	virtual StelPluginInfo getPluginInfo() const Q_DECL_OVERRIDE;
	virtual QObjectList getExtensionList() const Q_DECL_OVERRIDE { return QObjectList(); }
};

#endif /* CALENDARS_HPP */
