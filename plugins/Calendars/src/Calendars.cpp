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

#include "StelPainter.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModuleMgr.hpp"
#include "Calendars.hpp"

#include <QDebug>
//#include "StelGui.hpp"
#include "StelGuiItems.hpp"

#include "JulianCalendar.hpp"
#include "GregorianCalendar.hpp"
#include "MayaLongCountCalendar.hpp"
#include "MayaHaabCalendar.hpp"
#include "MayaTzolkinCalendar.hpp"

#include "CalendarsDialog.hpp"
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
	info.authors = "Stellarium team";
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
	flagShowChinese(true),
	flagShowMayaLongCount(true),
	flagShowMayaHaab(true),
	flagShowMayaTzolkin(true)
{
	setObjectName("Calendars");
	font.setPixelSize(15);

	configDialog = new CalendarsDialog();
	conf = StelApp::getInstance().getSettings();
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
			qDebug() << "button...";
			toolbarButton = new StelButton(Q_NULLPTR,
						       QPixmap(":/Calendars/bt_Calendars_On.png"),
						       QPixmap(":/Calendars/bt_Calendars_Off.png"),
						       QPixmap(":/graphicGui/miscGlow32x32.png"),
						       "actionShow_Calendars");
			qDebug() << "add button...";
			gui->getButtonBar()->addButton(toolbarButton, "065-pluginsGroup");
		}
	}
	catch (std::runtime_error& e)
	{
		qWarning() << "WARNING: unable to create toolbar button for Calendars plugin: " << e.what();
	}


	const double jd=StelApp::getInstance().getCore()->getJD();
	calendars.insert("Julian", new JulianCalendar(jd));
	calendars.insert("Gregorian", new GregorianCalendar(jd));
	calendars.insert("MayaLongCount", new MayaLongCountCalendar(jd));
	calendars.insert("MayaHaab", new MayaHaabCalendar(jd));
	calendars.insert("MayaTzolkin", new MayaTzolkinCalendar(jd));
	// TODO: Add your Calendar subclasses here.

	foreach (Calendar* cal, calendars)
	{
		connect(cal, SIGNAL(jdChanged(double)), StelApp::getInstance().getCore(), SLOT(setJD(double)));
	}
}


void Calendars::loadSettings()
{
	// Now activate calendar displays if needed.
	enable(           conf->value("Calendars/show", true).toBool());
	showJulian(       conf->value("Calendars/show_julian", true).toBool());
	showGregorian(    conf->value("Calendars/show_gregorian", true).toBool());
	showISO(          conf->value("Calendars/show_iso", true).toBool());
	showChinese(      conf->value("Calendars/show_chinese", false).toBool());
	showMayaLongCount(conf->value("Calendars/show_maya_long_count", false).toBool());
	showMayaHaab(     conf->value("Calendars/show_maya_haab", false).toBool());
	showMayaTzolkin(  conf->value("Calendars/show_maya_tzolkin", false).toBool());
}

void Calendars::restoreDefaultSettings()
{
	Q_ASSERT(conf);
	// Remove the old values...
	conf->remove("Calendars");
	// ...load the default values...
	loadSettings();
}

/******************************************************************************
 Draw our module. This should print the activated calendars in the main window
 TODO: select activated only, better formatting (table?), screen target coordinates
*******************************************************************************/
void Calendars::draw(StelCore* core)
{
	if (!enabled) return;

	// TODO: Replace this horrible text output by a nice HTML table on a QGraphicsTextItem. (See SkyGui/InfoPanel!)
	StelPainter painter(core->getProjection2d());
	painter.setColor(1,1,1,1);
	painter.setFont(font);

	// TODO: Select the drawable calendars from GUI or settings.
	if (calendars.count()==0) return;
	if (flagShowJulian) painter.drawText(1300, 870, QString("Julian: ") + getCal("Julian")->getFormattedDateString());
	if (flagShowGregorian) painter.drawText(1300, 855, QString("Gregorian: ") + getCal("Gregorian")->getFormattedDateString());
	if (flagShowISO) painter.drawText(1300, 840, QString("ISO week: ")); // + getCal("ISO")->getFormattedDateString());
	if (flagShowMayaLongCount) painter.drawText(1300, 825, QString("Maya Long Count: ") + getCal("MayaLongCount")->getFormattedDateString());
	if (flagShowMayaHaab) painter.drawText(1300, 810, QString("Maya Haab: ") + getCal("MayaHaab")->getFormattedDateString());
	if (flagShowMayaTzolkin) painter.drawText(1300, 795, QString("Maya Tzolkin: ") + getCal("MayaTzolkin")->getFormattedDateString());
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
		conf->setValue("Calendars/show_julian", b);
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
