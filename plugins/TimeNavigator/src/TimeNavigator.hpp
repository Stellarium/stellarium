/*
 * Time Navigator plug-in for Stellarium
 *
 * Copyright (C) 2026 Atque
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

#ifndef TIMENAVIGATOR_HPP
#define TIMENAVIGATOR_HPP

#include "StelGui.hpp"
#include "StelModule.hpp"

#include <QString>

class QPixmap;
class StelButton;
class TimeNavigatorDialog;

/*! @defgroup timeNavigator Time Navigator plug-in
@{
The Time Navigator plugin provides a convenient GUI window for stepping
the simulation time forward or backward by common astronomical periods
(solar day, sidereal day, synodic month, tropical year, Saros, etc.),
as well as for jumping to specific events for the selected object
(rising, transit, setting) and twilight times.

<b>Configuration</b>

The plug-in's configuration data is stored in Stellarium's main
configuration file (section [TimeNavigator]).

@}
*/

//! @class TimeNavigator
//! @ingroup timeNavigator
//! Main class of the Time Navigator plug-in.
class TimeNavigator : public StelModule
{
	Q_OBJECT
	Q_PROPERTY(bool flagShowButton
		   READ getFlagShowButton
		   WRITE setFlagShowButton
		   NOTIFY flagShowButtonChanged)

public:
	TimeNavigator();
	~TimeNavigator() override;

	void init() override;
	void deinit() override;
	double getCallOrder(StelModuleActionName actionName) const override;
	bool configureGui(bool show=true) override;

	//! Restore the plug-in's settings to the default state.
	void restoreDefaults();

	//! Load the plug-in's settings from the configuration file.
	void readSettingsFromConfig();

	//! Save the settings to the main configuration file.
	void saveSettingsToConfig() const;

	bool getFlagShowButton() const { return flagShowButton; }

public slots:
	//! Display/hide the toolbar button for this plugin.
	void setFlagShowButton(bool b);

signals:
	void flagShowButtonChanged(bool b);

private slots:
	//! Called when the user clicks "Save settings" in the dialog.
	void saveSettings() { saveSettingsToConfig(); }

private:
	void restoreDefaultConfigIni();

	TimeNavigatorDialog* mainWindow;
	QSettings* conf;
	StelGui* gui;

	bool flagShowButton;
	StelButton* toolbarButton;
};


#include <QObject>
#include "StelPluginInterface.hpp"

//! This class is used by Qt to manage a plug-in interface
class TimeNavigatorStelPluginInterface : public QObject, public StelPluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID StelPluginInterface_iid)
	Q_INTERFACES(StelPluginInterface)
public:
	StelModule* getStelModule() const override;
	StelPluginInfo getPluginInfo() const override;
};

#endif /* TIMENAVIGATOR_HPP */
