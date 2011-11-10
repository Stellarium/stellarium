/*
 * Time zone configuration plug-in for Stellarium
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

#include "TimeZoneConfiguration.hpp"
#include "TimeZoneConfigurationWindow.hpp"

#include "StelApp.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModule.hpp"

#include <QSettings>

StelModule* TimeZoneConfigurationStelPluginInterface::getStelModule() const
{
	return new TimeZoneConfiguration();
}

StelPluginInfo TimeZoneConfigurationStelPluginInterface::getPluginInfo() const
{
	// Allow to load the resources when used as a static plugin
	//Q_INIT_RESOURCE(TimeZoneConfiguration);

	StelPluginInfo info;
	info.id = "TimeZoneConfiguration";
	info.displayedName = N_("Time Zone");
	info.authors = "Bogdan Marinov";
	info.contact = "http://stellarium.org";
	info.description = N_("A convenient interface for some of the more obscure options in Stellarium's configuration file. Allows setting the time zone and changing the way the time and the date are displayed in the bottom bar.");
	return info;
}

Q_EXPORT_PLUGIN2(TimeZoneConfiguration, TimeZoneConfigurationStelPluginInterface);

TimeZoneConfiguration::TimeZoneConfiguration()
{
	setObjectName("TimeZoneConfiguration");

	//
}

TimeZoneConfiguration::~TimeZoneConfiguration()
{
	//
}

void TimeZoneConfiguration::init()
{
	mainWindow = new TimeZoneConfigurationWindow();
}

void TimeZoneConfiguration::deinit()
{
	delete mainWindow;
}

double TimeZoneConfiguration::getCallOrder(StelModuleActionName) const
{
	return 0.;
}

void TimeZoneConfiguration::update(double)
{
	//
}

bool TimeZoneConfiguration::configureGui(bool show)
{
	if (show)
		mainWindow->setVisible(true);

	return true;
}

void TimeZoneConfiguration::setTimeZone(QString timeZoneString)
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

QString TimeZoneConfiguration::readTimeZone()
{
	QSettings * settings = StelApp::getInstance().getSettings();
	Q_ASSERT(settings);

	return settings->value("localization/time_zone").toString();
}

void TimeZoneConfiguration::saveDisplayFormats()
{
	QSettings * settings = StelApp::getInstance().getSettings();
	Q_ASSERT(settings);

	StelLocaleMgr & localeManager = StelApp::getInstance().getLocaleMgr();

	QString timeDisplayFormat = localeManager.getTimeFormatStr();
	settings->setValue("localization/time_display_format", timeDisplayFormat);

	QString dateDisplayFormat = localeManager.getDateFormatStr();
	settings->setValue("localization/date_display_format", dateDisplayFormat);

	settings->sync();
}
