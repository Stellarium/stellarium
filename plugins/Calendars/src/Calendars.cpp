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

#include <stdexcept>
#include <QDebug>
#include <QStyleOptionGraphicsItem>
#include <QPainter>
#include "StelGuiItems.hpp"
#include "StelPainter.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "SkyGui.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelTranslator.hpp"

#include "Calendars.hpp"
#include "CalendarsDialog.hpp"

#include "JulianCalendar.hpp"
#include "GregorianCalendar.hpp"
#include "ISOCalendar.hpp"
#include "IcelandicCalendar.hpp"
#include "RomanCalendar.hpp"
#include "OlympicCalendar.hpp"
#include "EgyptianCalendar.hpp"
#include "ArmenianCalendar.hpp"
#include "ZoroastrianCalendar.hpp"
#include "CopticCalendar.hpp"
#include "EthiopicCalendar.hpp"
#include "IslamicCalendar.hpp"
#include "HebrewCalendar.hpp"
#include "OldHinduSolarCalendar.hpp"
#include "OldHinduLuniSolarCalendar.hpp"
#include "MayaLongCountCalendar.hpp"
#include "MayaHaabCalendar.hpp"
#include "MayaTzolkinCalendar.hpp"
#include "AztecXihuitlCalendar.hpp"
#include "AztecTonalpohualliCalendar.hpp"
#include "BalinesePawukonCalendar.hpp"
#include "FrenchArithmeticCalendar.hpp"
#include "PersianArithmeticCalendar.hpp"

/*************************************************************************
 This method is the one called automatically by the StelModuleMgr just 
 after loading the dynamic library
*************************************************************************/
StelModule* CalendarsStelPluginInterface::getStelModule() const
{
	return new Calendars();
}

StelPluginInfo CalendarsStelPluginInterface::getPluginInfo() const
{
	// Allow to load the resources when used as a static plugin
	Q_INIT_RESOURCE(Calendars);

	StelPluginInfo info;
	info.id = "Calendars";
	info.displayedName = N_("Calendars");
	info.authors = "Georg Zotti";
	info.contact = "www.stellarium.org";
	info.description = N_("Calendars of the world");
	info.version = CALENDARS_PLUGIN_VERSION;
	info.license = CALENDARS_PLUGIN_LICENSE;
	return info;
}

/*************************************************************************
 Constructor
*************************************************************************/
Calendars::Calendars():
	toolbarButton(Q_NULLPTR),
	enabled(true),
	flagShowJulian(true),
	flagShowGregorian(true),
	flagShowISO(true),
	flagShowIcelandic(true),
	flagShowRoman(true),
	flagShowOlympic(true),
	flagShowEgyptian(true),
	flagShowArmenian(true),
	flagShowZoroastrian(true),
	flagShowCoptic(true),
	flagShowEthiopic(true),
	flagShowChinese(true),
	flagShowIslamic(true),
	flagShowHebrew(true),
	flagShowOldHinduSolar(true),
	flagShowOldHinduLunar(true),
	flagShowMayaLongCount(true),
	flagShowMayaHaab(true),
	flagShowMayaTzolkin(true),
	flagShowAztecXihuitl(true),
	flagShowAztecTonalpohualli(true),
	flagShowBalinese(true),
	flagShowFrenchArithmetic(true),
	flagShowPersianArithmetic(true)
{
	setObjectName("Calendars");
	font.setPixelSize(15);

	configDialog = new CalendarsDialog();
	conf = StelApp::getInstance().getSettings();

	infoPanel=new CalendarsInfoPanel(this, static_cast<StelGui*>(StelApp::getInstance().getGui())->getSkyGui());
}

/*************************************************************************
 Destructor
*************************************************************************/
Calendars::~Calendars()
{
	delete configDialog; configDialog=Q_NULLPTR;
	foreach (QString key, calendars.keys())
	{
		Calendar *cal = calendars.take(key);
		delete cal;
	}
	if (infoPanel) delete infoPanel;
}

bool Calendars::configureGui(bool show)
{
	if (show)
		configDialog->setVisible(true);
	return true;
}
/*************************************************************************
 Reimplementation of the getCallOrder method
*************************************************************************/
double Calendars::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("LandscapeMgr")->getCallOrder(actionName)+10.;
	return 0;
}


/*************************************************************************
 Init our module
*************************************************************************/
void Calendars::init()
{
	loadSettings();
	// Create action for enable/disable & hook up signals
	QString section=N_("Calendars");
	addAction("actionShow_Calendars",         section, N_("Calendars"), "enabled", "Alt+K");
	addAction("actionShow_Calendars_dialog",  section, N_("Show settings dialog"),  configDialog,  "visible",           "Alt+Shift+K");

	// Add a toolbar button
	StelApp& app=StelApp::getInstance();
	try
	{
		StelGui* gui = dynamic_cast<StelGui*>(app.getGui());
		if (gui!=Q_NULLPTR)
		{
			//qDebug() << "button...";
			toolbarButton = new StelButton(Q_NULLPTR,
						       QPixmap(":/Calendars/bt_Calendars_On.png"),
						       QPixmap(":/Calendars/bt_Calendars_Off.png"),
						       QPixmap(":/graphicGui/miscGlow32x32.png"),
						       "actionShow_Calendars");
			//qDebug() << "add button...";
			gui->getButtonBar()->addButton(toolbarButton, "065-pluginsGroup");
		}
	}
	catch (std::runtime_error& e)
	{
		qWarning() << "WARNING: unable to create toolbar button for Calendars plugin: " << e.what();
	}

	infoPanel->setFont(font);
	infoPanel->setPos(600, 300);

	const double jd=StelApp::getInstance().getCore()->getJD();
	calendars.insert("Julian", new JulianCalendar(jd));
	calendars.insert("Gregorian", new GregorianCalendar(jd));
	calendars.insert("ISO", new ISOCalendar(jd));
	calendars.insert("Icelandic", new IcelandicCalendar(jd));
	calendars.insert("Roman", new RomanCalendar(jd));
	calendars.insert("Olympic", new OlympicCalendar(jd));
	calendars.insert("Egyptian", new EgyptianCalendar(jd));
	calendars.insert("Armenian", new ArmenianCalendar(jd));
	calendars.insert("Zoroastrian", new ZoroastrianCalendar(jd));
	calendars.insert("Coptic", new CopticCalendar(jd));
	calendars.insert("Ethiopic", new EthiopicCalendar(jd));
	calendars.insert("Islamic", new IslamicCalendar(jd));
	calendars.insert("Hebrew", new HebrewCalendar(jd));
	calendars.insert("OldHinduSolar", new OldHinduSolarCalendar(jd));
	calendars.insert("OldHinduLunar", new OldHinduLuniSolarCalendar(jd));
	calendars.insert("MayaLongCount", new MayaLongCountCalendar(jd));
	calendars.insert("MayaHaab", new MayaHaabCalendar(jd));
	calendars.insert("MayaTzolkin", new MayaTzolkinCalendar(jd));
	calendars.insert("AztecXihuitl", new AztecXihuitlCalendar(jd));
	calendars.insert("AztecTonalpohualli", new AztecTonalpohualliCalendar(jd));
	calendars.insert("Balinese", new BalinesePawukonCalendar(jd));
	calendars.insert("FrenchArithmetic", new FrenchArithmeticCalendar(jd));
	calendars.insert("PersianArithmetic", new PersianArithmeticCalendar(jd));
	// TODO: Add your Calendar subclasses here.

	foreach (Calendar* cal, calendars)
	{
		connect(cal, SIGNAL(jdChanged(double)), StelApp::getInstance().getCore(), SLOT(setJD(double)));
		connect(&StelApp::getInstance(), SIGNAL(languageChanged()), cal, SLOT(retranslate()));
	}
}

void Calendars::loadSettings()
{
	// Now activate calendar displays if needed.
	enable(                conf->value("Calendars/show", true).toBool());
	showJulian(            conf->value("Calendars/show_julian", true).toBool());
	showGregorian(         conf->value("Calendars/show_gregorian", true).toBool());
	showISO(               conf->value("Calendars/show_iso", true).toBool());
	showIcelandic(         conf->value("Calendars/show_icelandic", true).toBool());
	showRoman(             conf->value("Calendars/show_roman", true).toBool());
	showOlympic(           conf->value("Calendars/show_olympic", true).toBool());
	showEgyptian(          conf->value("Calendars/show_egyptian", true).toBool());
	showArmenian(          conf->value("Calendars/show_armenian", true).toBool());
	showZoroastrian(       conf->value("Calendars/show_zoroastrian", true).toBool());
	showCoptic(	       conf->value("Calendars/show_coptic", true).toBool());
	showEthiopic(	       conf->value("Calendars/show_ethiopic", true).toBool());
	showChinese(           conf->value("Calendars/show_chinese", false).toBool());
	showIslamic(           conf->value("Calendars/show_islamic", true).toBool());
	showHebrew(            conf->value("Calendars/show_hebrew", true).toBool());
	showOldHinduSolar(     conf->value("Calendars/show_old_hindu_solar", true).toBool());
	showOldHinduLunar(     conf->value("Calendars/show_old_hindu_lunar", true).toBool());
	showMayaLongCount(     conf->value("Calendars/show_maya_long_count", true).toBool());
	showMayaHaab(          conf->value("Calendars/show_maya_haab", true).toBool());
	showMayaTzolkin(       conf->value("Calendars/show_maya_tzolkin", true).toBool());
	showAztecXihuitl(      conf->value("Calendars/show_aztec_xihuitl", true).toBool());
	showAztecTonalpohualli(conf->value("Calendars/show_aztec_tonalpohualli", true).toBool());
	showBalinese(          conf->value("Calendars/show_balinese_pawukon", true).toBool());
	showFrenchArithmetic(  conf->value("Calendars/show_french_arithmetic", true).toBool());
	showPersianArithmetic( conf->value("Calendars/show_persian_arithmetic", true).toBool());
}

void Calendars::restoreDefaultSettings()
{
	Q_ASSERT(conf);
	// Remove the old values...
	conf->remove("Calendars");
	// ...load the default values...
	loadSettings();
}

/*********************************************************************************
 Draw our module. This tabulates the activated calendars in the lower right corner
**********************************************************************************/
void Calendars::draw(StelCore* core)
{
	if (!enabled)
	{
		infoPanel->hide();
		return;
	}

	QString str;
	QTextStream oss(&str);
	oss << "<table>";
	// Select the drawable calendars from GUI or settings.
	if (calendars.count()==0) return;
	if (flagShowJulian)        oss << QString("<tr><td>%1</td><td>%2</td></tr>").arg(qc_("Julian",          "calendar")).arg(getCal("Julian")->getFormattedDateString());
	if (flagShowGregorian)     oss << QString("<tr><td>%1</td><td>%2</td></tr>").arg(qc_("Gregorian",       "calendar")).arg(getCal("Gregorian")->getFormattedDateString());
	if (flagShowISO)           oss << QString("<tr><td>%1</td><td>%2</td></tr>").arg(qc_("ISO week",        "calendar")).arg(getCal("ISO")->getFormattedDateString());
	if (flagShowIcelandic)     oss << QString("<tr><td>%1</td><td>%2</td></tr>").arg(qc_("Icelandic",       "calendar")).arg(getCal("Icelandic")->getFormattedDateString());
	if (flagShowRoman)         oss << QString("<tr><td>%1</td><td>%2</td></tr>").arg(qc_("Roman",           "calendar")).arg(getCal("Roman")->getFormattedDateString());
	if (flagShowOlympic)       oss << QString("<tr><td>%1</td><td>%2</td></tr>").arg(qc_("Olympic",         "calendar")).arg(getCal("Olympic")->getFormattedDateString());
	if (flagShowEgyptian)      oss << QString("<tr><td>%1</td><td>%2</td></tr>").arg(qc_("Egyptian",        "calendar")).arg(getCal("Egyptian")->getFormattedDateString());
	if (flagShowArmenian)      oss << QString("<tr><td>%1</td><td>%2</td></tr>").arg(qc_("Armenian",        "calendar")).arg(getCal("Armenian")->getFormattedDateString());
	if (flagShowZoroastrian)   oss << QString("<tr><td>%1</td><td>%2</td></tr>").arg(qc_("Zoroastrian",     "calendar")).arg(getCal("Zoroastrian")->getFormattedDateString());
	if (flagShowCoptic)        oss << QString("<tr><td>%1</td><td>%2</td></tr>").arg(qc_("Coptic",          "calendar")).arg(getCal("Coptic")->getFormattedDateString());
	if (flagShowEthiopic)      oss << QString("<tr><td>%1</td><td>%2</td></tr>").arg(qc_("Ethiopic",        "calendar")).arg(getCal("Ethiopic")->getFormattedDateString());
	if (flagShowFrenchArithmetic) oss << QString("<tr><td>%1</td><td>%2</td></tr>").arg(qc_("French Rev. (Arithm.)",   "calendar")).arg(getCal("FrenchArithmetic")->getFormattedDateString());
	if (flagShowIslamic)       oss << QString("<tr><td>%1</td><td>%2</td></tr>").arg(qc_("Islamic",         "calendar")).arg(getCal("Islamic")->getFormattedDateString());
	if (flagShowHebrew)        oss << QString("<tr><td>%1</td><td>%2</td></tr>").arg(qc_("Hebrew",          "calendar")).arg(getCal("Hebrew")->getFormattedDateString());
	if (flagShowPersianArithmetic) oss << QString("<tr><td>%1</td><td>%2</td></tr>").arg(qc_("Persian (Arithm.)", "calendar")).arg(getCal("PersianArithmetic")->getFormattedDateString());
	if (flagShowOldHinduSolar) oss << QString("<tr><td>%1</td><td>%2</td></tr>").arg(qc_("Old Hindu Solar", "calendar")).arg(getCal("OldHinduSolar")->getFormattedDateString());
	if (flagShowOldHinduLunar) oss << QString("<tr><td>%1</td><td>%2</td></tr>").arg(qc_("Old Hindu Lunisolar", "calendar")).arg(getCal("OldHinduLunar")->getFormattedDateString());
	if (flagShowMayaLongCount) oss << QString("<tr><td>%1</td><td>%2</td></tr>").arg(qc_("Maya Long Count", "calendar")).arg(getCal("MayaLongCount")->getFormattedDateString());
	if (flagShowMayaHaab)      oss << QString("<tr><td>%1</td><td>%2</td></tr>").arg(qc_("Maya Haab",       "calendar")).arg(getCal("MayaHaab")->getFormattedDateString());
	if (flagShowMayaTzolkin)   oss << QString("<tr><td>%1</td><td>%2</td></tr>").arg(qc_("Maya Tzolkin",    "calendar")).arg(getCal("MayaTzolkin")->getFormattedDateString());
	if (flagShowAztecXihuitl)  oss << QString("<tr><td>%1</td><td>%2</td></tr>").arg(qc_("Aztec Xihuitl",   "calendar")).arg(getCal("AztecXihuitl")->getFormattedDateString());
	if (flagShowAztecTonalpohualli) oss << QString("<tr><td>%1</td><td>%2</td></tr>").arg(qc_("Aztec Tonalpohualli", "calendar")).arg(getCal("AztecTonalpohualli")->getFormattedDateString());
	if (flagShowBalinese)      {
		oss << QString("<tr><td>%1</td><td>%2</td></tr>").arg(qc_("Balinese Pawukon", "calendar")).arg(static_cast<BalinesePawukonCalendar*>(getCal("Balinese"))->getFormattedDateString1to5());
		oss << QString("<tr><td>%1</td><td>%2</td></tr>").arg("").arg(static_cast<BalinesePawukonCalendar*>(getCal("Balinese"))->getFormattedDateString6to10());
	}
	oss << "</table>";
	Vec3f color(1);
	if (StelApp::getInstance().getFlagOverwriteInfoColor())
	{
		// make info text more readable...
		color = StelApp::getInstance().getOverwriteInfoColor();
	}
	if (core->isBrightDaylight() && !StelApp::getInstance().getVisionModeNight())
	{
		// make info text more readable when atmosphere enabled at daylight.
		color = StelApp::getInstance().getDaylightInfoColor();
	}

	infoPanel->setDefaultTextColor(color.toQColor());
	infoPanel->setHtml(str);
	infoPanel->updatePosition();
	infoPanel->show();
}

// Get a pointer to the respective calendar
Calendar* Calendars::getCal(QString name)
{
	return calendars.value(name, Q_NULLPTR);
}

void Calendars::update(double)
{
	const double jd=StelApp::getInstance().getCore()->getJD();
	foreach (Calendar* cal, calendars)
	{
		cal->setJD(jd);
	}
}

/*****************************************************************************
 * Boilerplate: Setters/getters.
 * */

bool Calendars::isEnabled() const {return enabled;}
void Calendars::enable(bool b)
{
	if (b!=enabled)
	{
		enabled=b;
		conf->setValue("Calendars/show", b);
		emit enabledChanged(b);
	}
}
bool Calendars::isJulianDisplayed() const { return flagShowJulian;}
void Calendars::showJulian(bool b)
{
	if (b!=flagShowJulian)
	{
		flagShowJulian=b;
		conf->setValue("Calendars/show_julian", b);
		emit showJulianChanged(b);
	}
}
bool Calendars::isGregorianDisplayed() const { return flagShowGregorian;}
void Calendars::showGregorian(bool b)
{
	if (b!=flagShowGregorian)
	{
		flagShowGregorian=b;
		conf->setValue("Calendars/show_gregorian", b);
		emit showGregorianChanged(b);
	}
}
bool Calendars::isISODisplayed() const { return flagShowISO;}
void Calendars::showISO(bool b)
{
	if (b!=flagShowISO)
	{
		flagShowISO=b;
		conf->setValue("Calendars/show_iso", b);
		emit showISOChanged(b);
	}
}

bool Calendars::isIcelandicDisplayed() const { return flagShowIcelandic;}
void Calendars::showIcelandic(bool b)
{
	if (b!=flagShowIcelandic)
	{
		flagShowIcelandic=b;
		conf->setValue("Calendars/show_icelandic", b);
		emit showIcelandicChanged(b);
	}
}

bool Calendars::isRomanDisplayed() const { return flagShowRoman;}
void Calendars::showRoman(bool b)
{
	if (b!=flagShowRoman)
	{
		flagShowRoman=b;
		conf->setValue("Calendars/show_roman", b);
		emit showRomanChanged(b);
	}
}

bool Calendars::isOlympicDisplayed() const { return flagShowOlympic;}
void Calendars::showOlympic(bool b)
{
	if (b!=flagShowOlympic)
	{
		flagShowOlympic=b;
		conf->setValue("Calendars/show_olympic", b);
		emit showOlympicChanged(b);
	}
}

bool Calendars::isEgyptianDisplayed() const { return flagShowEgyptian;}
void Calendars::showEgyptian(bool b)
{
	if (b!=flagShowEgyptian)
	{
		flagShowEgyptian=b;
		conf->setValue("Calendars/show_egyptian", b);
		emit showEgyptianChanged(b);
	}
}

bool Calendars::isArmenianDisplayed() const { return flagShowArmenian;}
void Calendars::showArmenian(bool b)
{
	if (b!=flagShowArmenian)
	{
		flagShowArmenian=b;
		conf->setValue("Calendars/show_armenian", b);
		emit showArmenianChanged(b);
	}
}

bool Calendars::isZoroastrianDisplayed() const { return flagShowZoroastrian;}
void Calendars::showZoroastrian(bool b)
{
	if (b!=flagShowZoroastrian)
	{
		flagShowZoroastrian=b;
		conf->setValue("Calendars/show_zoroastrian", b);
		emit showZoroastrianChanged(b);
	}
}

bool Calendars::isCopticDisplayed() const { return flagShowCoptic;}
void Calendars::showCoptic(bool b)
{
	if (b!=flagShowCoptic)
	{
		flagShowCoptic=b;
		conf->setValue("Calendars/show_coptic", b);
		emit showCopticChanged(b);
	}
}

bool Calendars::isEthiopicDisplayed() const { return flagShowEthiopic;}
void Calendars::showEthiopic(bool b)
{
	if (b!=flagShowEthiopic)
	{
		flagShowEthiopic=b;
		conf->setValue("Calendars/show_ethiopic", b);
		emit showEthiopicChanged(b);
	}
}

bool Calendars::isChineseDisplayed() const { return flagShowChinese;}
void Calendars::showChinese(bool b)
{
	if (b!=flagShowChinese)
	{
		flagShowChinese=b;
		conf->setValue("Calendars/show_chinese", b);
		emit showChineseChanged(b);
	}
}

bool Calendars::isIslamicDisplayed() const { return flagShowIslamic;}
void Calendars::showIslamic(bool b)
{
	if (b!=flagShowIslamic)
	{
		flagShowIslamic=b;
		conf->setValue("Calendars/show_islamic", b);
		emit showIslamicChanged(b);
	}
}

bool Calendars::isHebrewDisplayed() const { return flagShowHebrew;}
void Calendars::showHebrew(bool b)
{
	if (b!=flagShowHebrew)
	{
		flagShowHebrew=b;
		conf->setValue("Calendars/show_hebrew", b);
		emit showHebrewChanged(b);
	}
}

bool Calendars::isOldHinduSolarDisplayed() const { return flagShowOldHinduSolar;}
void Calendars::showOldHinduSolar(bool b)
{
	if (b!=flagShowOldHinduSolar)
	{
		flagShowOldHinduSolar=b;
		conf->setValue("Calendars/show_old_hindu_solar", b);
		emit showOldHinduSolarChanged(b);
	}
}

bool Calendars::isOldHinduLunarDisplayed() const { return flagShowOldHinduLunar;}
void Calendars::showOldHinduLunar(bool b)
{
	if (b!=flagShowOldHinduLunar)
	{
		flagShowOldHinduLunar=b;
		conf->setValue("Calendars/show_old_hindu_lunar", b);
		emit showOldHinduLunarChanged(b);
	}
}

bool Calendars::isMayaLongCountDisplayed() const { return flagShowMayaLongCount;}
void Calendars::showMayaLongCount(bool b)
{
	if (b!=flagShowMayaLongCount)
	{
		flagShowMayaLongCount=b;
		conf->setValue("Calendars/show_maya_long_count", b);
		emit showMayaLongCountChanged(b);
	}
}

bool Calendars::isMayaHaabDisplayed() const { return flagShowMayaHaab;}
void Calendars::showMayaHaab(bool b)
{
	if (b!=flagShowMayaHaab)
	{
		flagShowMayaHaab=b;
		conf->setValue("Calendars/show_maya_haab", b);
		emit showMayaHaabChanged(b);
	}
}

bool Calendars::isMayaTzolkinDisplayed() const { return flagShowMayaTzolkin;}
void Calendars::showMayaTzolkin(bool b)
{
	if (b!=flagShowMayaTzolkin)
	{
		flagShowMayaTzolkin=b;
		conf->setValue("Calendars/show_maya_tzolkin", b);
		emit showMayaTzolkinChanged(b);
	}
}

bool Calendars::isAztecXihuitlDisplayed() const { return flagShowAztecXihuitl;}
void Calendars::showAztecXihuitl(bool b)
{
	if (b!=flagShowAztecXihuitl)
	{
		flagShowAztecXihuitl=b;
		conf->setValue("Calendars/show_aztec_xihuitl", b);
		emit showAztecXihuitlChanged(b);
	}
}

bool Calendars::isAztecTonalpohualliDisplayed() const { return flagShowAztecTonalpohualli;}
void Calendars::showAztecTonalpohualli(bool b)
{
	if (b!=flagShowAztecTonalpohualli)
	{
		flagShowAztecTonalpohualli=b;
		conf->setValue("Calendars/show_aztec_tonalpohualli", b);
		emit showAztecTonalpohualliChanged(b);
	}
}

bool Calendars::isBalineseDisplayed() const { return flagShowBalinese;}
void Calendars::showBalinese(bool b)
{
	if (b!=flagShowBalinese)
	{
		flagShowBalinese=b;
		conf->setValue("Calendars/show_balinese_pawukon", b);
		emit showBalineseChanged(b);
	}
}

bool Calendars::isFrenchArithmeticDisplayed() const { return flagShowFrenchArithmetic;}
void Calendars::showFrenchArithmetic(bool b)
{
	if (b!=flagShowFrenchArithmetic)
	{
		flagShowFrenchArithmetic=b;
		conf->setValue("Calendars/show_french_arithmetic", b);
		emit showFrenchArithmeticChanged(b);
	}
}

bool Calendars::isPersianArithmeticDisplayed() const { return flagShowPersianArithmetic;}
void Calendars::showPersianArithmetic(bool b)
{
	if (b!=flagShowPersianArithmetic)
	{
		flagShowPersianArithmetic=b;
		conf->setValue("Calendars/show_persian_arithmetic", b);
		emit showPersianArithmeticChanged(b);
	}
}
