/*
 * Equation Of Time plug-in for Stellarium
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

#ifndef EQUATIONOFTIME_HPP
#define EQUATIONOFTIME_HPP

#include "StelGui.hpp"
#include "StelModule.hpp"

#include <QFont>
#include <QString>

class QPixmap;
class StelButton;
class EquationOfTimeWindow;

/*! @defgroup equationOfTime Equation of Time plug-in
@{
The %Equation of Time plugin shows the solution of the equation of time.

The equation of time describes the discrepancy between two kinds of solar
time. These are apparent solar time, which directly tracks the motion of
the sun, and mean solar time, which tracks a fictitious "mean" sun with
noons 24 hours apart. There is no universally accepted definition of the
sign of the equation of time. Some publications show it as positive when
a sundial is ahead of a clock; others when the clock is ahead of the sundial.
In the English-speaking world, the former usage is the more common, but is
not always followed. Anyone who makes use of a published table or graph
should first check its sign usage.

<b>Configuration</b>

The plug-ins' configuration data is stored in Stellarium's main configuration
file (section [EquationOfTime]).

@}
*/

//! @class EquationOfTime
//! @ingroup equationOfTime
//! Main class of the %Equation of Time plugin.
//! @author Alexander Wolf
class EquationOfTime : public StelModule
{
	Q_OBJECT
	Q_PROPERTY(bool showEOT
		   READ isEnabled
		   WRITE enableEquationOfTime
		   NOTIFY equationOfTimeStateChanged)
	Q_PROPERTY(Vec3f textColor
		   READ getTextColor
		   WRITE setTextColor
		   NOTIFY textColorChanged
		   )

public:
	EquationOfTime();
	virtual ~EquationOfTime();

	virtual void init();
	virtual void deinit();
	virtual void update(double) {;}
	virtual void draw(StelCore *core);
	virtual double getCallOrder(StelModuleActionName actionName) const;
	virtual bool configureGui(bool show);

	//! Set up the plugin with default values.  This means clearing out the EquationOfTime section in the
	//! main config.ini (if one already exists), and populating it with default values.
	void restoreDefaults(void);

	//! Read (or re-read) settings from the main config file.  This will be called from init and also
	//! when restoring defaults (i.e. from the configuration dialog / restore defaults button).
	void readSettingsFromConfig(void);

	//! Save the settings to the main configuration file.
	void saveSettingsToConfig(void) const;

	//! Is plugin enabled?
	bool isEnabled() const
	{
		return flagShowSolutionEquationOfTime;
	}

	//! Get font size for messages
	int getFontSize(void) const
	{
		return fontSize;
	}
	//! Get status of usage minutes and seconds for value of equation
	bool getFlagMsFormat(void) const
	{
		return flagUseMsFormat;
	}
	//! Get status of usage inverted values for equation of time
	bool getFlagInvertedValue(void) const
	{
		return flagUseInvertedValue;
	}
	bool getFlagEnableAtStartup(void) const
	{
		return flagEnableAtStartup;
	}
	bool getFlagShowEOTButton(void) const
	{
		return flagShowEOTButton;
	}

public slots:
	//! Enable plugin usage
	void enableEquationOfTime(bool b);
	//! Enable usage inverted value for equation of time (switch sign of equation)
	void setFlagInvertedValue(bool b);
	//! Enable usage minutes and seconds for value
	void setFlagMsFormat(bool b);
	//! Enable plugin usage at startup
	void setFlagEnableAtStartup(bool b);
	//! Set font size for message
	void setFontSize(int size);
	//! Display plugin button on toolbar
	void setFlagShowEOTButton(bool b);

	Vec3f getTextColor() const { return textColor; }
	void setTextColor(const Vec3f& c) { textColor=c; }

private slots:
	void updateMessageText();
	//! Call when button "Save settings" in main GUI are pressed
	void saveSettings() { saveSettingsToConfig(); }

signals:
	void equationOfTimeStateChanged(bool b);
	void textColorChanged(Vec3f);

private:
	// if existing, delete EquationOfTime section in main config.ini, then create with default values
	void restoreDefaultConfigIni(void);

	EquationOfTimeWindow* mainWindow;
	QSettings* conf;
	StelGui* gui;

	QFont font;
	bool flagShowSolutionEquationOfTime;
	bool flagUseInvertedValue;
	bool flagUseMsFormat;
	bool flagEnableAtStartup;
	bool flagShowEOTButton;
	QString messageEquation;
	QString messageEquationMinutes;
	QString messageEquationSeconds;
	Vec3f textColor;
	int fontSize;
	StelButton* toolbarButton;
};


#include <QObject>
#include "StelPluginInterface.hpp"

//! This class is used by Qt to manage a plug-in interface
class EquationOfTimeStelPluginInterface : public QObject, public StelPluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID StelPluginInterface_iid)
	Q_INTERFACES(StelPluginInterface)
public:
	virtual StelModule* getStelModule() const;
	virtual StelPluginInfo getPluginInfo() const;
	virtual QObjectList getExtensionList() const { return QObjectList(); }
};

#endif /* EQUATIONOFTIME_HPP */
