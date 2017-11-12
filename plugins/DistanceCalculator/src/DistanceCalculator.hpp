/*
 * Distance Calculator plug-in for Stellarium
 *
 * Copyright (C) 2017 Alexander Wolf
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

#ifndef _DISTANCECALCULATOR_HPP_
#define _DISTANCECALCULATOR_HPP_

#include "StelGui.hpp"
#include "StelModule.hpp"

#include <QString>

class DistanceCalculatorWindow;

/*! @defgroup distanceCalculator Distance Calculator Plug-in
@{
The %Distance Calculator plugin allows measure linear distance between Solar system bodies.

<b>Configuration</b>

The plug-ins' configuration data is stored in Stellarium's main configuration
file (section [DistanceCalculator]).

@}
*/

//! @class DistanceCalculator
//! Main class of the %Distance Calculator plugin.
//! @author Alexander Wolf
//! @ingroup distanceCalculator
class DistanceCalculator : public StelModule
{
	Q_OBJECT

public:
	DistanceCalculator();
	~DistanceCalculator();

	virtual void init();
	virtual void deinit();
	virtual void update(double deltaTime);
	virtual double getCallOrder(StelModuleActionName actionName) const;
	virtual bool configureGui(bool show);

	//! Set up the plugin with default values.  This means clearing out the Distance section in the
	//! main config.ini (if one already exists), and populating it with default values.
	void restoreDefaults(void);

	//! Read (or re-read) settings from the main config file.  This will be called from init and also
	//! when restoring defaults (i.e. from the configuration dialog / restore defaults button).
	void readSettingsFromConfig(void);

	//! Save the settings to the main configuration file.
	void saveSettingsToConfig(void);
	
	void setFirstCelestialBody(QString body) { firstCelestialBody=body; }
	QString getFirstCelestialBody(void) { return firstCelestialBody; }
	
	void setSecondCelestialBody(QString body) { secondCelestialBody=body; }
	QString getSecondCelestialBody(void) { return secondCelestialBody; }

private:
	QString firstCelestialBody, secondCelestialBody;
	// if existing, delete Satellites section in main config.ini, then create with default values
	void restoreDefaultConfigIni(void);

	DistanceCalculatorWindow* distanceCalculatorDialog;
	QSettings* conf;
};


#include <QObject>
#include "StelPluginInterface.hpp"

//! This class is used by Qt to manage a plug-in interface
class DistanceCalculatorStelPluginInterface : public QObject, public StelPluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID StelPluginInterface_iid)
	Q_INTERFACES(StelPluginInterface)
public:
	virtual StelModule* getStelModule() const;
	virtual StelPluginInfo getPluginInfo() const;
	virtual QObjectList getExtensionList() const { return QObjectList(); }
};

#endif /* _DISTANCECALCULATOR_HPP_ */
