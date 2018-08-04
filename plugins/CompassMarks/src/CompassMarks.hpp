/*
 * Copyright (C) 2009 Matthew Gates
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
 
#ifndef COMPASSMARKS_HPP
#define COMPASSMARKS_HPP

#include "VecMath.hpp"
#include "StelModule.hpp"
#include "StelFader.hpp"
#include <QFont>

class StelButton;

/*! @defgroup compassMarks Compass Marks Plug-in
@{
Stellarium helps the user get their bearings using the cardinal point
feature - the North, South, East and West markers on the horizon.
CompassMarks takes this idea and extends it to add markings every few
degrees along the horizon, and includes compass bearing values in degrees.
@}
*/

//! Main class of the Compass Marks plug-in.
//! Provides a ring of marks indicating azimuth on the horizon,
//! like a compass dial.
//! @author Matthew Gates
//! @ingroup compassMarks
class CompassMarks : public StelModule
{
	Q_OBJECT
	Q_PROPERTY(bool marksVisible READ getCompassMarks WRITE setCompassMarks NOTIFY compassMarksChanged)
public:
	CompassMarks();
	virtual ~CompassMarks();
	
	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	virtual void init();
	virtual void update(double deltaTime);
	virtual void draw(StelCore* core);
	virtual double getCallOrder(StelModuleActionName actionName) const;

	//! Load the plug-in's settings from the configuration file.
	//! Settings are kept in the "CompassMarks" section in Stellarium's
	//! configuration file. If no such section exists, it will load default
	//! values.
	//! @see restoreDefaultSettings()
	void loadConfiguration();

	void restoreDefaultConfiguration();

public slots:
	//! Get flag for displaying a ring of marks indicating azimuth on the horizon.
	bool getCompassMarks() const {return markFader;}

	//! Define whether a ring of azimuth marks on the horizon should be visible.
	//! @param b if true, the ring of azimuth marks is visible, else not
	void setCompassMarks(bool b);	

signals:
	void compassMarksChanged(bool);

private slots:
	void cardinalPointsChanged(bool b);

private:
	QSettings* conf;
	//! Whether the marks should be displayed at startup.
	bool displayedAtStartup;
	//! Font used for displaying bearing numbers.
	QFont font;
	Vec3f markColor;
	LinearFader markFader;
	StelButton* toolbarButton;
	bool cardinalPointsState;
};


#include <QObject>
#include "StelPluginInterface.hpp"

//! This class is used by Qt to manage a plug-in interface
class CompassMarksStelPluginInterface : public QObject, public StelPluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID StelPluginInterface_iid)
	Q_INTERFACES(StelPluginInterface)
public:
	virtual StelModule* getStelModule() const;
	virtual StelPluginInfo getPluginInfo() const;
	virtual QObjectList getExtensionList() const { return QObjectList(); }
};

#endif /*COMPASSMARKS_HPP*/
