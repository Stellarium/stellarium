/*
 * Time zone manager plug-in for Stellarium
 *
 * Copyright (C) 2010 Bogdan Marinov
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "TimeZoneManager.hpp"
#include "TimeZoneManagerWindow.hpp"

#include "StelApp.hpp"
#include "StelModule.hpp"

#include <QSettings>

StelModule* TimeZoneManagerStelPluginInterface::getStelModule() const
{
	return new TimeZoneManager();
}

StelPluginInfo TimeZoneManagerStelPluginInterface::getPluginInfo() const
{
	// Allow to load the resources when used as a static plugin
	//Q_INIT_RESOURCE(TimeZoneManager);

	StelPluginInfo info;
	info.id = "TimeZoneManager";
	info.displayedName = "Time Zone";
	info.authors = "Bogdan Marinov";
	info.contact = "http://stellarium.org";
	info.description = "This plug-in allows the user to set the time zone used in Stellarium.";
	return info;
}

Q_EXPORT_PLUGIN2(TimeZoneManager, TimeZoneManagerStelPluginInterface);

TimeZoneManager::TimeZoneManager()
{
	setObjectName("TimeZoneManager");

	//
}

TimeZoneManager::~TimeZoneManager()
{
	//
}

void TimeZoneManager::init()
{
	mainWindow = new TimeZoneManagerWindow();
}

void TimeZoneManager::deinit()
{
	delete mainWindow;
}

double TimeZoneManager::getCallOrder(StelModuleActionName) const
{
	return 0.;
}

void TimeZoneManager::update(double)
{
	//
}

bool TimeZoneManager::configureGui(bool show)
{
	if (show)
		mainWindow->setVisible(true);

	return true;
}

void TimeZoneManager::setTimeZone(QString timeZoneString)
{
	if (timeZoneString.isEmpty())
	{
		return;
	}
	//TODO: Validation?
	qDebug() << "Setting \"localization/time_zone\" to" << timeZoneString;

	QSettings * settings = StelApp::getInstance().getSettings();
	Q_ASSERT(settings);

	settings->setValue("localization/time_zone", timeZoneString);
	settings->sync();
}

QString TimeZoneManager::readTimeZone()
{
	QSettings * settings = StelApp::getInstance().getSettings();
	Q_ASSERT(settings);

	return settings->value("localization/time_zone").toString();
}
