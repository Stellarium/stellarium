/*
 * Coordinates plug-in for Stellarium
 *
 * Copyright (C) 2014 Alexander Wolf
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

#ifndef _COORDINATES_HPP_
#define _COORDINATES_HPP_

#include "StelGui.hpp"
#include "StelModule.hpp"

#include <QFont>
#include <QString>

class QPixmap;
class StelButton;
class CoordinatesWindow;

class Coordinates : public StelModule
{
	Q_OBJECT
	Q_PROPERTY(bool enabled
		   READ isEnabled
		   WRITE enableCoordinates)

public:
	Coordinates();
	virtual ~Coordinates();

	virtual void init();
	virtual void deinit();
	virtual void update(double) {;}
	virtual void draw(StelCore *core);
	virtual double getCallOrder(StelModuleActionName actionName) const;
	virtual bool configureGui(bool show);

	//! Set up the plugin with default values.  This means clearing out the Pulsars section in the
	//! main config.ini (if one already exists), and populating it with default values.
	void restoreDefaults(void);

	//! Read (or re-read) settings from the main config file.  This will be called from init and also
	//! when restoring defaults (i.e. from the configuration dialog / restore defaults button).
	void readSettingsFromConfig(void);

	//! Save the settings to the main configuration file.
	void saveSettingsToConfig(void);

	//! Is plugin enabled?
	bool isEnabled() const
	{
		return flagShowCoordinates;
	}

	//! Get font size for messages
	int getFontSize(void)
	{ 
		return fontSize;
	}
	bool getFlagEnableAtStartup(void)
	{ 
		return flagEnableAtStartup;
	}
	bool getFlagShowCoordinatesButton(void)
	{
		return flagShowCoordinatesButton;
	}

public slots:
	//! Enable plugin usage
	void enableCoordinates(bool b);
	//! Enable plugin usage at startup
	void setFlagEnableAtStartup(bool b)
	{ 
		flagEnableAtStartup=b;
	}
	//! Set font size for message
	void setFontSize(int size)
	{ 
		fontSize=size;
	}
	//! Display plugin button on toolbar
	void setFlagShowCoordinatesButton(bool b);

private slots:
	void updateMessageText();

private:
	// if existing, delete coordinates section in main config.ini, then create with default values
	void restoreDefaultConfigIni(void);

	CoordinatesWindow* mainWindow;	
	QSettings* conf;
	StelGui* gui;

	QFont font;
	bool flagShowCoordinates;
	bool flagEnableAtStartup;
	bool flagShowCoordinatesButton;
	QString messageCoordinates;
	Vec3f textColor;
	Vec3d coordinatesPoint;
	int fontSize;
	StelButton* toolbarButton;
};


#include <QObject>
#include "StelPluginInterface.hpp"

//! This class is used by Qt to manage a plug-in interface
class CoordinatesStelPluginInterface : public QObject, public StelPluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "stellarium.StelGuiPluginInterface/1.0")
	Q_INTERFACES(StelPluginInterface)
public:
	virtual StelModule* getStelModule() const;
	virtual StelPluginInfo getPluginInfo() const;
};

#endif /* _COORDINATES_HPP_ */
