/*
 * Pointer Coordinates plug-in for Stellarium
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

#ifndef POINTERCOORDINATES_HPP
#define POINTERCOORDINATES_HPP

#include "StelGui.hpp"
#include "StelModule.hpp"

#include <QFont>
#include <QString>
#include <QPair>

class QPixmap;
class StelButton;
class PointerCoordinatesWindow;

/*! @defgroup pointerCoordinates Pointer Coordinates Plug-in
@{
The %Pointer Coordinates plugin shows the coordinates of the mouse pointer.

<b>Configuration</b>

The plug-ins' configuration data is stored in Stellarium's main configuration
file (section [PointerCoordinates]).

@}
*/

//! @class PointerCoordinates
//! Main class of the %Pointer Coordinates plugin.
//! @author Alexander Wolf
//! @ingroup pointerCoordinates
class PointerCoordinates : public StelModule
{
	Q_OBJECT
	Q_ENUMS(CoordinatesPlace)
	Q_ENUMS(CoordinateSystem)
	Q_PROPERTY(bool enabled
		   READ isEnabled
		   WRITE enableCoordinates
		   NOTIFY flagCoordinatesVisibilityChanged
		   )

public:
	//! @enum CoordinatesPlace
	//! Available places of string with coordinates
	enum CoordinatesPlace
	{
		TopCenter,		//!< The top center of the screen
		TopRight,		//!< In center of the top right half of the screen
		RightBottomCorner,	//!< The right bottom corner of the screen
		NearMouseCursor,	//!< Near mouse cursor
		Custom			//!< The custom position on the screen
	};

	//! @enum CoordinateSystem
	//! Available coordinate systems
	enum CoordinateSystem
	{
		RaDecJ2000,
		RaDec,
		HourAngle,
		Ecliptic,
		EclipticJ2000,
		AltAzi,
		Galactic,
		Supergalactic
	};


	PointerCoordinates();
	virtual ~PointerCoordinates();

	virtual void init();
	virtual void deinit();
	virtual void update(double) {;}
	virtual void draw(StelCore *core);
	virtual double getCallOrder(StelModuleActionName actionName) const;
	virtual bool configureGui(bool show);

	//! Set up the plugin with default values.  This means clearing out the PointerCoordinates section in the
	//! main config.ini (if one already exists), and populating it with default values.
	void restoreDefaultConfiguration(void);

	//! Read (or re-read) settings from the main config file.  This will be called from init and also
	//! when restoring defaults (i.e. from the configuration dialog / restore defaults button).
	void loadConfiguration(void);

	//! Save the settings to the main configuration file.
	void saveConfiguration(void);

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
	bool getFlagShowCrossedLines(void)
	{
		return flagShowCrossedLines;
	}

	QPair<int, int> getCoordinatesPlace(QString text);

	QPair<int, int> getCustomCoordinatesPlace()
	{
		return customPosition;
	}

signals:
	void flagCoordinatesVisibilityChanged(bool b);

private slots:
	//! Call when button "Save settings" in main GUI are pressed
	void saveSettings() { saveConfiguration(); }

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

	void setFlagShowCrossedLines(bool b)
	{
		flagShowCrossedLines=b;
	}

	//! Set the current place of the string with coordinates
	void setCurrentCoordinatesPlace(CoordinatesPlace place)
	{
		currentPlace = place;
	}
	//! Get the current place of the string with coordinates
	CoordinatesPlace getCurrentCoordinatesPlace() const
	{
		return currentPlace;
	}
	//! Get the current place of the string with coordinates
	QString getCurrentCoordinatesPlaceKey(void) const;
	//! Set the current place of the string with coordinates from its key
	void setCurrentCoordinatesPlaceKey(QString key);

	//! Set the current coordinate system
	void setCurrentCoordinateSystem(CoordinateSystem cs)
	{
		currentCoordinateSystem = cs;
	}
	//! Get the current coordinate system
	CoordinateSystem getCurrentCoordinateSystem() const
	{
		return currentCoordinateSystem;
	}
	//! Get the current coordinate system key
	QString getCurrentCoordinateSystemKey(void) const;
	//! Set the current coordinate system from its key
	void setCurrentCoordinateSystemKey(QString key);

	void setCustomCoordinatesPlace(int x, int y);

	void setFlagShowConstellation(bool b){flagShowConstellation=b;}
	bool getFlagShowConstellation(void) const {return flagShowConstellation;}
private:
	PointerCoordinatesWindow* mainWindow;
	QSettings* conf;
	StelGui* gui;

	// The current place for string with coordinates
	CoordinatesPlace currentPlace;
	// The current coordinate system
	CoordinateSystem currentCoordinateSystem;

	QFont font;
	bool flagShowCoordinates;
	bool flagEnableAtStartup;
	bool flagShowCoordinatesButton;
	bool flagShowConstellation;
	bool flagShowCrossedLines;
	Vec3f textColor;
	Vec3d coordinatesPoint;
	int fontSize;
	StelButton* toolbarButton;
	QPair<int, int> customPosition;
};


#include <QObject>
#include "StelPluginInterface.hpp"

//! This class is used by Qt to manage a plug-in interface
class PointerCoordinatesStelPluginInterface : public QObject, public StelPluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID StelPluginInterface_iid)
	Q_INTERFACES(StelPluginInterface)
public:
	virtual StelModule* getStelModule() const;
	virtual StelPluginInfo getPluginInfo() const;
	virtual QObjectList getExtensionList() const { return QObjectList(); }
};

#endif /* POINTERCOORDINATES_HPP */
