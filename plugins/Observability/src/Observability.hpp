/*
 * Copyright (C) 2012 Ivan Marti-Vidal
 * Copyright (C) 2021 Thomas Heinrichs
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

#pragma once

#include "StelModule.hpp"
#include <QFont>
#include <QString>
#include <QPair>
#include "VecMath.hpp"
#include "SolarSystem.hpp"
#include "Planet.hpp"
#include "StelFader.hpp"


class QPixmap;
class StelButton;
class ObservabilityDialog;

class Observability : public StelModule
{
	Q_OBJECT
	Q_PROPERTY(bool enabled READ isEnabled WRITE enableReport NOTIFY flagReportVisibilityChanged)

public:
	Observability();
	~Observability();

	virtual void init();
	virtual void update(double);
	virtual void draw(StelCore* core);
	virtual double getCallOrder(StelModuleActionName actionName) const;
	virtual bool configureGui(bool show = true);

	//! Restore the plug-in's settings to the default state.
	//! Replace the plug-in's settings in Stellarium's configuration file
	//! with the default values and re-load them.
	//! Uses internally loadSettings() and saveSettings().
	void restoreDefaultSettings();
	void loadSettings();

	// Getters

	bool getShowBestNight() { return flagShowBestNight; }
	bool getShowGoodNights() { return flagShowGoodNights; }
	bool getShowToday() { return flagShowToday; }
	bool getShowFullMoon() { return flagShowFullMoon; }

	//! Get the user-defined Sun altitude at twilight.
	//! @returns A value in degrees.
	int getTwilightAltitude() { return twilightAltitudeDeg; }

	//! Get the user-defined altitude of the visual horizon.
	int getHorizonAltitude() { return horizonAltitudeDeg; }

signals:
	void flagReportVisibilityChanged(bool visible);
	void flagShowGoodNightsChanged(bool b);
	void flagShowBestNightChanged(bool b);
	void flagShowTodayChanged(bool b);
	void flagShowFullMoonChanged(bool b);
	void twilightAltitudeDegChanged(int alt);
	void horizonAltitudeDegChanged(int alt);


public slots:
	bool isEnabled() const { return flagShowReport; }

	void enableReport(bool enabled);

	void showGoodNights(bool b);
	void showBestNight(bool b);
	void showToday(bool b);
	void showFullMoon(bool b);
	void setTwilightAltitudeDeg(int alt);
	void setHorizonAltitudeDeg(int alt);

private:
	bool flagShowReport;
	bool flagShowGoodNights;
	bool flagShowBestNight;
	bool flagShowToday;
	bool flagShowFullMoon;
	int twilightAltitudeDeg;
	int horizonAltitudeDeg;

	QSettings* config;
	ObservabilityDialog* configDialog;
};

#include "StelPluginInterface.hpp"
#include <QObject>

//! This class is used by Qt to manage a plug-in interface
class ObservabilityStelPluginInterface : public QObject, public StelPluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID StelPluginInterface_iid)
	Q_INTERFACES(StelPluginInterface)
public:
	virtual StelModule* getStelModule() const;
	virtual StelPluginInfo getPluginInfo() const;
	virtual QObjectList getExtensionList() const { return QObjectList(); }
};