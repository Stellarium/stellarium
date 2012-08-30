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
 
 #ifndef _TIME_ZONE_CONFIGURATION_HPP_
 #define _TIME_ZONE_CONFIGURATION_HPP_

#include "StelGui.hpp"
#include "StelModule.hpp"

#include <QString>

class TimeZoneConfigurationWindow;

class TimeZoneConfiguration : public StelModule
{
	Q_OBJECT

public:
	TimeZoneConfiguration();
	~TimeZoneConfiguration();

	virtual void init();
	virtual void deinit();
	virtual void update(double deltaTime);
	//virtual void draw(StelCore *core, class StelRenderer* renderer);
	virtual double getCallOrder(StelModuleActionName actionName) const;
	virtual bool configureGui(bool show);

	//! Sets the time zone field in Stellarium's configuration file.
	//! To work, timeZoneString should be a valid value of the TZ environmental
	//! variable according to the POSIX format.
	//! (See http://www.gnu.org/s/libc/manual/html_node/TZ-Variable.html)
	//! Note that Microsoft Windows may use a truncated version of the TZ
	//! format.
	//! For now, Stellarium needs to be restarted for the new settings to take
	//! effect.
	void setTimeZone(QString timeZoneString);

	//! Reads the time zone field in Stellarium's configuration file.
	//! \returns the contents of the "localization/time_zone" field in
	//! Stellarium's configuration file.
	QString readTimeZone();

	//! Saves the formats used to display date and time in the bottom bar.
	void saveDisplayFormats();

private:
	TimeZoneConfigurationWindow * mainWindow;
};

#include "fixx11h.h"
#include <QObject>
#include "StelPluginInterface.hpp"

//! This class is used by Qt to manage a plug-in interface
class TimeZoneConfigurationStelPluginInterface : public QObject, public StelPluginInterface
{
	Q_OBJECT
	Q_INTERFACES(StelPluginInterface)
public:
	virtual StelModule* getStelModule() const;
	virtual StelPluginInfo getPluginInfo() const;
};
 
 #endif //_TIME_ZONE_CONFIGURATION_HPP_
