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
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelGui.hpp"
#include "SkyGui.hpp"
#include "StelModuleMgr.hpp"
#include "StelTranslator.hpp"
#include "StelScriptMgr.hpp"

#include "Calendars.hpp"
#include "CalendarsDialog.hpp"

#include "JulianCalendar.hpp"
#include "RevisedJulianCalendar.hpp"
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
#include "NewHinduCalendar.hpp"
#include "NewHinduLunarCalendar.hpp"
#include "AstroHinduSolarCalendar.hpp"
#include "AstroHinduLunarCalendar.hpp"
#include "TibetanCalendar.hpp"
#include "MayaLongCountCalendar.hpp"
#include "MayaHaabCalendar.hpp"
#include "MayaTzolkinCalendar.hpp"
#include "AztecXihuitlCalendar.hpp"
#include "AztecTonalpohualliCalendar.hpp"
#include "BalinesePawukonCalendar.hpp"
#include "FrenchAstronomicalCalendar.hpp"
#include "FrenchArithmeticCalendar.hpp"
#include "PersianArithmeticCalendar.hpp"
#include "PersianAstronomicalCalendar.hpp"
#include "BahaiArithmeticCalendar.hpp"
#include "BahaiAstronomicalCalendar.hpp"
#include "ChineseCalendar.hpp"
#include "JapaneseCalendar.hpp"
#include "KoreanCalendar.hpp"
#include "VietnameseCalendar.hpp"

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
	info.contact = STELLARIUM_DEV_URL;
	info.description = N_("Calendars of the world");
	info.version = CALENDARS_PLUGIN_VERSION;
	info.license = CALENDARS_PLUGIN_LICENSE;
	return info;
}

/*************************************************************************
 Constructor
*************************************************************************/
Calendars::Calendars():
	toolbarButton(nullptr),
	enabled(true),
	flagTextColorOverride(false),
	textColor(0.75f),
	flagShowJulian(true),
	flagShowRevisedJulian(false),
	flagShowGregorian(true),
	flagShowISO(true),
	flagShowIcelandic(false),
	flagShowRoman(false),
	flagShowOlympic(false),
	flagShowEgyptian(false),
	flagShowArmenian(false),
	flagShowZoroastrian(false),
	flagShowCoptic(false),
	flagShowEthiopic(false),
	flagShowChinese(true),
	flagShowJapanese(true),
	flagShowKorean(true),
	flagShowVietnamese(true),
	flagShowIslamic(true),
	flagShowHebrew(true),
	flagShowOldHinduSolar(false),
	flagShowOldHinduLunar(false),
	flagShowNewHinduSolar(true),
	flagShowNewHinduLunar(true),
	flagShowAstroHinduSolar(false),
	flagShowAstroHinduLunar(false),
	flagShowMayaLongCount(true),
	flagShowMayaHaab(false),
	flagShowMayaTzolkin(false),
	flagShowAztecXihuitl(false),
	flagShowAztecTonalpohualli(false),
	flagShowBalinese(false),
	flagShowFrenchAstronomical(false),
	flagShowFrenchArithmetic(false),
	flagShowPersianArithmetic(false),
	flagShowPersianAstronomical(false),
	flagShowBahaiArithmetic(false),
	flagShowBahaiAstronomical(false),
	flagShowTibetan(false)
{
	setObjectName("Calendars");
	font.setPixelSize(StelApp::getInstance().getScreenFontSize());

	configDialog = new CalendarsDialog();
	conf = StelApp::getInstance().getSettings();

	infoPanel=new CalendarsInfoPanel(this, static_cast<StelGui*>(StelApp::getInstance().getGui())->getSkyGui());
	connect(&StelApp::getInstance(), &StelApp::screenFontSizeChanged, this, [=](int size){font.setPixelSize(size); infoPanel->setFont(font); infoPanel->updatePosition(true);});
}

/*************************************************************************
 Destructor
*************************************************************************/
Calendars::~Calendars()
{
	delete configDialog; configDialog=nullptr;
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
		if (gui)
		{
			//qDebug() << "button...";
			toolbarButton = new StelButton(nullptr,
						       QPixmap(":/Calendars/bt_Calendars_On.png"),
						       QPixmap(":/Calendars/bt_Calendars_Off.png"),
						       QPixmap(":/graphicGui/miscGlow32x32.png"),
						       "actionShow_Calendars",
						       false,
						       "actionShow_Calendars_dialog");
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
	calendars.insert("Calendar", new Calendar(jd)); // For scripting use only.
	calendars.insert("Julian", new JulianCalendar(jd));
	calendars.insert("RevisedJulian", new RevisedJulianCalendar(jd));
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
	calendars.insert("NewHinduSolar", new NewHinduCalendar(jd));
	calendars.insert("NewHinduLunar", new NewHinduLunarCalendar(jd));
	calendars.insert("AstroHinduSolar", new AstroHinduSolarCalendar(jd));
	calendars.insert("AstroHinduLunar", new AstroHinduLunarCalendar(jd));
	calendars.insert("MayaLongCount", new MayaLongCountCalendar(jd));
	calendars.insert("MayaHaab", new MayaHaabCalendar(jd));
	calendars.insert("MayaTzolkin", new MayaTzolkinCalendar(jd));
	calendars.insert("AztecXihuitl", new AztecXihuitlCalendar(jd));
	calendars.insert("AztecTonalpohualli", new AztecTonalpohualliCalendar(jd));
	calendars.insert("Balinese", new BalinesePawukonCalendar(jd));
	calendars.insert("FrenchAstronomical", new FrenchAstronomicalCalendar(jd));
	calendars.insert("FrenchArithmetic", new FrenchArithmeticCalendar(jd));
	calendars.insert("PersianArithmetic", new PersianArithmeticCalendar(jd));
	calendars.insert("PersianAstronomical", new PersianAstronomicalCalendar(jd));
	calendars.insert("BahaiArithmetic", new BahaiArithmeticCalendar(jd));
	calendars.insert("BahaiAstronomical", new BahaiAstronomicalCalendar(jd));
	calendars.insert("Tibetan", new TibetanCalendar(jd));
	calendars.insert("Chinese", new ChineseCalendar(jd));
	calendars.insert("Japanese", new JapaneseCalendar(jd));
	calendars.insert("Korean", new KoreanCalendar(jd));
	calendars.insert("Vietnamese", new VietnameseCalendar(jd));
	// TODO: Add your Calendar subclasses here.

	foreach (Calendar* cal, calendars)
	{
#ifndef NDEBUG
		// We must set JD explicitly to enforce creation of parts for a debug mode test in StelMainView::init().
		cal->setJD(jd);
#endif
		connect(cal, SIGNAL(jdChanged(double)), StelApp::getInstance().getCore(), SLOT(setJD(double)));
		connect(&StelApp::getInstance(), SIGNAL(languageChanged()), cal, SLOT(retranslate()));
	}
}

#ifdef ENABLE_SCRIPTING
// Add calendar as scriptable object! Some scripting functions won't work though, as they use object types unknown to the scripting engine.
void Calendars::makeCalendarsScriptable(StelScriptMgr *ssm)
{
	foreach (Calendar* cal, calendars)
	{
		cal->setObjectName(cal->metaObject()->className());
		ssm->addObject(cal);
	}
}
#endif

void Calendars::loadSettings()
{
	QString defaultColor =  conf->value("color/default_color", "0.5,0.5,0.7").toString();
	// Now activate calendar displays if needed.
	enable(                 conf->value("Calendars/show", true).toBool());
	setFlagTextColorOverride(conf->value("Calendars/flag_text_color_override", false).toBool());
	setTextColor(           Vec3f(conf->value("Calendars/text_color", defaultColor).toString()));
	showJulian(             conf->value("Calendars/show_julian", true).toBool());
	showRevisedJulian(      conf->value("Calendars/show_revised_julian", false).toBool());
	showGregorian(          conf->value("Calendars/show_gregorian", true).toBool());
	showISO(                conf->value("Calendars/show_iso", true).toBool());
	showIcelandic(          conf->value("Calendars/show_icelandic", false).toBool());
	showRoman(              conf->value("Calendars/show_roman", false).toBool());
	showOlympic(            conf->value("Calendars/show_olympic", false).toBool());
	showEgyptian(           conf->value("Calendars/show_egyptian", false).toBool());
	showArmenian(           conf->value("Calendars/show_armenian", false).toBool());
	showZoroastrian(        conf->value("Calendars/show_zoroastrian", false).toBool());
	showCoptic(	        conf->value("Calendars/show_coptic", false).toBool());
	showEthiopic(	        conf->value("Calendars/show_ethiopic", false).toBool());
	showChinese(            conf->value("Calendars/show_chinese", false).toBool());
	showJapanese(           conf->value("Calendars/show_japanese", false).toBool());
	showKorean(             conf->value("Calendars/show_korean", false).toBool());
	showVietnamese(         conf->value("Calendars/show_vietnamese", false).toBool());
	showIslamic(            conf->value("Calendars/show_islamic", true).toBool());
	showHebrew(             conf->value("Calendars/show_hebrew", true).toBool());
	showOldHinduSolar(      conf->value("Calendars/show_old_hindu_solar", false).toBool());
	showOldHinduLunar(      conf->value("Calendars/show_old_hindu_lunar", false).toBool());
	showNewHinduSolar(      conf->value("Calendars/show_new_hindu_solar", true).toBool());
	showNewHinduLunar(      conf->value("Calendars/show_new_hindu_lunar", true).toBool());
	showAstroHinduSolar(    conf->value("Calendars/show_astro_hindu_solar", false).toBool());
	showAstroHinduLunar(    conf->value("Calendars/show_astro_hindu_lunar", false).toBool());
	showMayaLongCount(      conf->value("Calendars/show_maya_long_count", true).toBool());
	showMayaHaab(           conf->value("Calendars/show_maya_haab", false).toBool());
	showMayaTzolkin(        conf->value("Calendars/show_maya_tzolkin", false).toBool());
	showAztecXihuitl(       conf->value("Calendars/show_aztec_xihuitl", false).toBool());
	showAztecTonalpohualli( conf->value("Calendars/show_aztec_tonalpohualli", false).toBool());
	showBalinese(           conf->value("Calendars/show_balinese_pawukon", false).toBool());
	showFrenchAstronomical( conf->value("Calendars/show_french_astronomical", false).toBool());
	showFrenchArithmetic(   conf->value("Calendars/show_french_arithmetic", false).toBool());
	showPersianArithmetic(  conf->value("Calendars/show_persian_arithmetic", false).toBool());
	showPersianAstronomical(conf->value("Calendars/show_persian_astronomical", false).toBool());
	showBahaiArithmetic(    conf->value("Calendars/show_bahai_arithmetic", false).toBool());
	showBahaiAstronomical(  conf->value("Calendars/show_bahai_astronomical", false).toBool());
	showTibetan(            conf->value("Calendars/show_tibetan", false).toBool());
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
	if (flagShowGregorian)          oss << QString("<tr><td>%1&nbsp;</td><td>%2</td></tr>").arg(qc_("Gregorian",             "calendar"), getCal("Gregorian")->getFormattedDateString());
	if (flagShowJulian)             oss << QString("<tr><td>%1&nbsp;</td><td>%2</td></tr>").arg(qc_("Julian",                "calendar"), getCal("Julian")->getFormattedDateString());
	if (flagShowRevisedJulian)      oss << QString("<tr><td>%1&nbsp;</td><td>%2</td></tr>").arg(qc_("Revised Julian",        "calendar"), getCal("RevisedJulian")->getFormattedDateString());
	if (flagShowISO)                oss << QString("<tr><td>%1&nbsp;</td><td>%2</td></tr>").arg(qc_("ISO week",              "calendar"), getCal("ISO")->getFormattedDateString());
	if (flagShowIcelandic)          oss << QString("<tr><td>%1&nbsp;</td><td>%2</td></tr>").arg(qc_("Icelandic",             "calendar"), getCal("Icelandic")->getFormattedDateString());
	if (flagShowRoman)              oss << QString("<tr><td>%1&nbsp;</td><td>%2</td></tr>").arg(qc_("Roman",                 "calendar"), getCal("Roman")->getFormattedDateString());
	if (flagShowOlympic)            oss << QString("<tr><td>%1&nbsp;</td><td>%2</td></tr>").arg(qc_("Olympic",               "calendar"), getCal("Olympic")->getFormattedDateString());
	if (flagShowEgyptian)           oss << QString("<tr><td>%1&nbsp;</td><td>%2</td></tr>").arg(qc_("Egyptian",              "calendar"), getCal("Egyptian")->getFormattedDateString());
	if (flagShowArmenian)           oss << QString("<tr><td>%1&nbsp;</td><td>%2</td></tr>").arg(qc_("Armenian",              "calendar"), getCal("Armenian")->getFormattedDateString());
	if (flagShowZoroastrian)        oss << QString("<tr><td>%1&nbsp;</td><td>%2</td></tr>").arg(qc_("Zoroastrian",           "calendar"), getCal("Zoroastrian")->getFormattedDateString());
	if (flagShowCoptic)             oss << QString("<tr><td>%1&nbsp;</td><td>%2</td></tr>").arg(qc_("Coptic",                "calendar"), getCal("Coptic")->getFormattedDateString());
	if (flagShowEthiopic)           oss << QString("<tr><td>%1&nbsp;</td><td>%2</td></tr>").arg(qc_("Ethiopic",              "calendar"), getCal("Ethiopic")->getFormattedDateString());
	if (flagShowFrenchAstronomical) oss << QString("<tr><td>%1&nbsp;</td><td>%2</td></tr>").arg(qc_("French Rev. (Astron.)", "calendar"), getCal("FrenchAstronomical")->getFormattedDateString());
	if (flagShowFrenchArithmetic)   oss << QString("<tr><td>%1&nbsp;</td><td>%2</td></tr>").arg(qc_("French Rev. (Arithm.)", "calendar"), getCal("FrenchArithmetic")->getFormattedDateString());
	if (flagShowIslamic)            oss << QString("<tr><td>%1&nbsp;</td><td>%2</td></tr>").arg(qc_("Islamic",               "calendar"), getCal("Islamic")->getFormattedDateString());
	if (flagShowHebrew)             oss << QString("<tr><td>%1&nbsp;</td><td>%2</td></tr>").arg(qc_("Hebrew",                "calendar"), getCal("Hebrew")->getFormattedDateString());
	if (flagShowPersianArithmetic)  oss << QString("<tr><td>%1&nbsp;</td><td>%2</td></tr>").arg(qc_("Persian (Arithm.)",     "calendar"), getCal("PersianArithmetic")->getFormattedDateString());
	if (flagShowPersianAstronomical)oss << QString("<tr><td>%1&nbsp;</td><td>%2</td></tr>").arg(qc_("Persian (Astron.)",     "calendar"), getCal("PersianAstronomical")->getFormattedDateString());
	if (flagShowBahaiArithmetic)    oss << QString("<tr><td>%1&nbsp;</td><td>%2</td></tr>").arg(qc_("Bahá’í (Arithm.)",      "calendar"), getCal("BahaiArithmetic")->getFormattedDateString());
	if (flagShowBahaiAstronomical)  oss << QString("<tr><td>%1&nbsp;</td><td>%2</td></tr>").arg(qc_("Bahá’í (Astron.)",      "calendar"), getCal("BahaiAstronomical")->getFormattedDateString());
	if (flagShowOldHinduSolar)      oss << QString("<tr><td>%1&nbsp;</td><td>%2</td></tr>").arg(qc_("Old Hindu Solar",       "calendar"), getCal("OldHinduSolar")->getFormattedDateString());
	if (flagShowOldHinduLunar)      oss << QString("<tr><td>%1&nbsp;</td><td>%2</td></tr>").arg(qc_("Old Hindu Lunisolar",   "calendar"), getCal("OldHinduLunar")->getFormattedDateString());
	if (flagShowNewHinduSolar)      oss << QString("<tr><td>%1&nbsp;</td><td>%2</td></tr>").arg(qc_("New Hindu Solar",       "calendar"), getCal("NewHinduSolar")->getFormattedDateString());
	if (flagShowNewHinduLunar)      oss << QString("<tr><td>%1&nbsp;</td><td>%2</td></tr>").arg(qc_("New Hindu Lunisolar",   "calendar"), getCal("NewHinduLunar")->getFormattedDateString());
	if (flagShowNewHinduLunar)      oss << QString("<tr><td>%1&nbsp;</td><td>%2</td></tr>").arg(qc_("New Hindu Panchang",    "calendar"),
												    static_cast<NewHinduLunarCalendar*>(getCal("NewHinduLunar"))->getFormattedPanchangString());
	if (flagShowAstroHinduSolar)    oss << QString("<tr><td>%1&nbsp;</td><td>%2</td></tr>").arg(qc_("Hindu Astro Solar",     "calendar"), getCal("AstroHinduSolar")->getFormattedDateString());
	if (flagShowAstroHinduLunar)    oss << QString("<tr><td>%1&nbsp;</td><td>%2</td></tr>").arg(qc_("Hindu Astro Lunisolar", "calendar"), getCal("AstroHinduLunar")->getFormattedDateString());
	if (flagShowTibetan)            oss << QString("<tr><td>%1&nbsp;</td><td>%2</td></tr>").arg(qc_("Tibetan",               "calendar"), getCal("Tibetan")->getFormattedDateString());
	if (flagShowChinese) {
					oss << QString("<tr><td>%1&nbsp;</td><td>%2</td></tr>").arg(qc_("Chinese",               "calendar"), getCal("Chinese")->getFormattedDateString());
					oss << QString("<tr><td>%1&nbsp;</td><td>%2</td></tr>").arg(qc_("Chinese Solar Terms",   "calendar"), static_cast<ChineseCalendar*>(getCal("Chinese"))->getFormattedSolarTermsString());
	}
	if (flagShowJapanese) {
					oss << QString("<tr><td>%1&nbsp;</td><td>%2</td></tr>").arg(qc_("Japanese",              "calendar"), getCal("Japanese")->getFormattedDateString());
					oss << QString("<tr><td>%1&nbsp;</td><td>%2</td></tr>").arg(qc_("Japanese Solar Terms",  "calendar"), static_cast<JapaneseCalendar*>(getCal("Japanese"))->getFormattedSolarTermsString());
	}
	if (flagShowKorean) {
					oss << QString("<tr><td>%1&nbsp;</td><td>%2</td></tr>").arg(qc_("Korean",                "calendar"), getCal("Korean")->getFormattedDateString());
					oss << QString("<tr><td>%1&nbsp;</td><td>%2</td></tr>").arg(qc_("Korean Solar Terms",    "calendar"), static_cast<KoreanCalendar*>(getCal("Korean"))->getFormattedSolarTermsString());
	}
	if (flagShowVietnamese) {
					oss << QString("<tr><td>%1&nbsp;</td><td>%2</td></tr>").arg(qc_("Vietnamese",            "calendar"), getCal("Vietnamese")->getFormattedDateString());
					oss << QString("<tr><td>%1&nbsp;</td><td>%2</td></tr>").arg(qc_("Vietnamese Solar Terms","calendar"), static_cast<VietnameseCalendar*>(getCal("Vietnamese"))->getFormattedSolarTermsString());
	}
	if (flagShowBalinese)           oss << QString("<tr><td>%1&nbsp;</td><td>%2</td></tr>").arg(qc_("Balinese Pawukon",      "calendar"), getCal("Balinese")->getFormattedDateString());
	if (flagShowMayaLongCount)      oss << QString("<tr><td>%1&nbsp;</td><td>%2</td></tr>").arg(qc_("Maya Long Count",       "calendar"), getCal("MayaLongCount")->getFormattedDateString());
	if (flagShowMayaHaab)           oss << QString("<tr><td>%1&nbsp;</td><td>%2</td></tr>").arg(qc_("Maya Haab",             "calendar"), getCal("MayaHaab")->getFormattedDateString());
	if (flagShowMayaTzolkin)        oss << QString("<tr><td>%1&nbsp;</td><td>%2</td></tr>").arg(qc_("Maya Tzolkin",          "calendar"), getCal("MayaTzolkin")->getFormattedDateString());
	if (flagShowAztecXihuitl)       oss << QString("<tr><td>%1&nbsp;</td><td>%2</td></tr>").arg(qc_("Aztec Xihuitl",         "calendar"), getCal("AztecXihuitl")->getFormattedDateString());
	if (flagShowAztecTonalpohualli) oss << QString("<tr><td>%1&nbsp;</td><td>%2</td></tr>").arg(qc_("Aztec Tonalpohualli",   "calendar"), getCal("AztecTonalpohualli")->getFormattedDateString());
	oss << "</table>";
	Vec3f color(1);
	if (getFlagTextColorOverride())
	{
		color=textColor;
	}
	else
	{
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
	}

	infoPanel->setDefaultTextColor(color.toQColor());
	infoPanel->setHtml(str);
	infoPanel->updatePosition();
	infoPanel->show();
}

// Get a pointer to the respective calendar
Calendar* Calendars::getCal(QString name)
{
	return calendars.value(name, nullptr);
}

void Calendars::update(double)
{
	if (!enabled)
		return;
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
bool Calendars::isRevisedJulianDisplayed() const { return flagShowRevisedJulian;}
void Calendars::showRevisedJulian(bool b)
{
	if (b!=flagShowRevisedJulian)
	{
		flagShowRevisedJulian=b;
		conf->setValue("Calendars/show_revised_julian", b);
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

bool Calendars::isJapaneseDisplayed() const { return flagShowJapanese;}
void Calendars::showJapanese(bool b)
{
	if (b!=flagShowJapanese)
	{
		flagShowJapanese=b;
		conf->setValue("Calendars/show_japanese", b);
		emit showJapaneseChanged(b);
	}
}

bool Calendars::isKoreanDisplayed() const { return flagShowKorean;}
void Calendars::showKorean(bool b)
{
	if (b!=flagShowKorean)
	{
		flagShowKorean=b;
		conf->setValue("Calendars/show_korean", b);
		emit showKoreanChanged(b);
	}
}

bool Calendars::isVietnameseDisplayed() const { return flagShowVietnamese;}
void Calendars::showVietnamese(bool b)
{
	if (b!=flagShowVietnamese)
	{
		flagShowVietnamese=b;
		conf->setValue("Calendars/show_vietnamese", b);
		emit showVietnameseChanged(b);
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

bool Calendars::isNewHinduSolarDisplayed() const { return flagShowNewHinduSolar;}
void Calendars::showNewHinduSolar(bool b)
{
	if (b!=flagShowNewHinduSolar)
	{
		flagShowNewHinduSolar=b;
		conf->setValue("Calendars/show_new_hindu_solar", b);
		emit showNewHinduSolarChanged(b);
	}
}

bool Calendars::isNewHinduLunarDisplayed() const { return flagShowNewHinduLunar;}
void Calendars::showNewHinduLunar(bool b)
{
	if (b!=flagShowNewHinduLunar)
	{
		flagShowNewHinduLunar=b;
		conf->setValue("Calendars/show_new_hindu_lunar", b);
		emit showNewHinduLunarChanged(b);
	}
}

bool Calendars::isAstroHinduSolarDisplayed() const { return flagShowAstroHinduSolar;}
void Calendars::showAstroHinduSolar(bool b)
{
	if (b!=flagShowAstroHinduSolar)
	{
		flagShowAstroHinduSolar=b;
		conf->setValue("Calendars/show_astro_hindu_solar", b);
		emit showAstroHinduSolarChanged(b);
	}
}

bool Calendars::isAstroHinduLunarDisplayed() const { return flagShowAstroHinduLunar;}
void Calendars::showAstroHinduLunar(bool b)
{
	if (b!=flagShowAstroHinduLunar)
	{
		flagShowAstroHinduLunar=b;
		conf->setValue("Calendars/show_astro_hindu_lunar", b);
		emit showAstroHinduLunarChanged(b);
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

bool Calendars::isFrenchAstronomicalDisplayed() const { return flagShowFrenchAstronomical;}
void Calendars::showFrenchAstronomical(bool b)
{
	if (b!=flagShowFrenchAstronomical)
	{
		flagShowFrenchAstronomical=b;
		conf->setValue("Calendars/show_french_astronomical", b);
		emit showFrenchAstronomicalChanged(b);
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

bool Calendars::isPersianAstronomicalDisplayed() const { return flagShowPersianAstronomical;}
void Calendars::showPersianAstronomical(bool b)
{
	if (b!=flagShowPersianAstronomical)
	{
		flagShowPersianAstronomical=b;
		conf->setValue("Calendars/show_persian_astronomical", b);
		emit showPersianAstronomicalChanged(b);
	}
}

bool Calendars::isBahaiArithmeticDisplayed() const { return flagShowBahaiArithmetic;}
void Calendars::showBahaiArithmetic(bool b)
{
	if (b!=flagShowBahaiArithmetic)
	{
		flagShowBahaiArithmetic=b;
		conf->setValue("Calendars/show_bahai_arithmetic", b);
		emit showBahaiArithmeticChanged(b);
	}
}

bool Calendars::isBahaiAstronomicalDisplayed() const { return flagShowBahaiAstronomical;}
void Calendars::showBahaiAstronomical(bool b)
{
	if (b!=flagShowBahaiAstronomical)
	{
		flagShowBahaiAstronomical=b;
		conf->setValue("Calendars/show_bahai_astronomical", b);
		emit showBahaiAstronomicalChanged(b);
	}
}

bool Calendars::isTibetanDisplayed() const { return flagShowTibetan;}
void Calendars::showTibetan(bool b)
{
	if (b!=flagShowTibetan)
	{
		flagShowTibetan=b;
		conf->setValue("Calendars/show_tibetan", b);
		emit showTibetanChanged(b);
	}
}

bool Calendars::getFlagTextColorOverride() const { return flagTextColorOverride;}
void Calendars::setFlagTextColorOverride(bool b)
{
	if (b!=flagTextColorOverride)
	{
		flagTextColorOverride=b;
		conf->setValue("Calendars/flag_text_color_override", b);
		emit flagTextColorOverrideChanged(b);
	}
}

Vec3f Calendars::getTextColor() const
{
	return textColor;
}
void Calendars::setTextColor(const Vec3f& newColor)
{
	if(newColor != textColor)
	{
		textColor=newColor;
		emit textColorChanged(textColor);
	}
}
