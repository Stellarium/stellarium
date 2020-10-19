/*
 * Copyright (C) 2020 Ray Wang
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

// TODO select calendars in a popup configuration dialog to display.

#include "StelProjector.hpp"
#include "StelPainter.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "SkyGui.hpp"
#include "StelUtils.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelGui.hpp"
#include "StelGuiItems.hpp"
#include "ExtraCalendar.hpp"
#include "Gregorian.hpp"
#include "Calendars/Chinese.hpp"

#include <QDebug>

/*************************************************************************
 This method is the one called automatically by the StelModuleMgr just
 after loading the dynamic library
*************************************************************************/
StelModule* ExtraCalendarStelPluginInterface::getStelModule() const
{
	return new ExtraCalendar();
}

StelPluginInfo ExtraCalendarStelPluginInterface::getPluginInfo() const
{
	StelPluginInfo info;
	info.id = "ExtraCalendar";
	info.displayedName = N_("Extra Calendar");
	info.authors = "Ray Wang";
	info.contact = "ray.hackmylife@gmail.com";
	info.description = N_("Display various calendars. Currently support Chinese calendar (1900-2100).");
	info.version = EXTRACALENDAR_PLUGIN_VERSION;
	info.license = EXTRACALENDAR_PLUGIN_LICENSE;
	return info;
}

/*************************************************************************
 Constructor
*************************************************************************/
ExtraCalendar::ExtraCalendar()
{
	setObjectName("ExtraCalendar");
	font.setPixelSize(14);
	StelApp &app = StelApp::getInstance();
	gui = dynamic_cast<StelGui*>(app.getGui());
}

/*************************************************************************
 Destructor
*************************************************************************/
ExtraCalendar::~ExtraCalendar()
{
}

/*************************************************************************
 Reimplementation of the getCallOrder method
*************************************************************************/
double ExtraCalendar::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("LabelMgr")->getCallOrder(actionName)+10.;
	return 0;
}

/*************************************************************************
 Init our module
*************************************************************************/
void ExtraCalendar::init()
{
	qDebug() << "init called for ExtraCalendar";
}

/*************************************************************************
 Draw the calendar text on the screen
*************************************************************************/
void ExtraCalendar::draw(StelCore* core)
{
	// Get current Julian datetime and treat it as UTC
	QDateTime datetime = StelUtils::jdToQDateTime(core->getJD());
	datetime.setTimeSpec(Qt::UTC);
	QDate date = datetime.toLocalTime().date();

	// Make a gregorian date from JD
	Gregorian* g = new Gregorian();
	g->year = date.year();
	g->month = date.month();
	g->day = date.day();

	QString text;
	// Append available calendars. Currently only Chinese calendar is available.
	text += getChineseCalendarDateString(g);

	QFontMetrics fm(font);
	QSize fs = fm.size(Qt::TextSingleLine, text);

	StelPainter painter(core->getProjection2d());
	painter.setColor(1,1,1,1);
	painter.setFont(font);
	// Draw text at the bottom-right corner
	painter.drawText(
   	gui->getSkyGui()->getSkyGuiWidth() * 1.8 - fs.width(),
    140,
		text);

	delete g;
}
