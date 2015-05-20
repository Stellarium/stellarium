/*
 * FOV plug-in for Stellarium
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

#ifndef _FOV_HPP_
#define _FOV_HPP_

#include "StelGui.hpp"
#include "StelModule.hpp"

#include <QString>

class FOVWindow;

/*! @mainpage notitle
@section overview Plugin Overview

The %Field of View plugin allows stepwise zooming via keyboard shortcuts like
in the Cartes du Ciel planetarium program.

@section config Configuration
The plug-ins' configuration data is stored in Stellarium's main configuration
file (section [FOV]).
*/

//! @class FOV
//! Main class of the %Field of View plugin.
//! @author Alexander Wolf
class FOV : public StelModule
{
	Q_OBJECT

public:
	FOV();
	~FOV();

	virtual void init();
	virtual void deinit();
	virtual void update(double deltaTime);
	//virtual void draw(StelCore *core);
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

	void setQuickFOV(const double value, const int item);
	double getQuickFOV(const int item) const;

public slots:
	void setFOV();

private:
	// if existing, delete Satellites section in main config.ini, then create with default values
	void restoreDefaultConfigIni(void);

	QList<double> FOVitem, FOVdefault;

	FOVWindow* mainWindow;
	QSettings* conf;	
};


#include <QObject>
#include "StelPluginInterface.hpp"

//! This class is used by Qt to manage a plug-in interface
class FOVStelPluginInterface : public QObject, public StelPluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID StelPluginInterface_iid)
	Q_INTERFACES(StelPluginInterface)
public:
	virtual StelModule* getStelModule() const;
	virtual StelPluginInfo getPluginInfo() const;
};

#endif /* _FOV_HPP_ */
