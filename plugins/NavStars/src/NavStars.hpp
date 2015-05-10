/*
 * Navigational Stars plug-in
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#ifndef _NAVSTARS_HPP_
#define _NAVSTARS_HPP_

#include "StelFader.hpp"
#include "StelModule.hpp"
#include "StelObject.hpp" // For StelObjectP
#include "StelTexture.hpp"

#include <QSettings>

class StelButton;
class StelPainter;
class StarMgr;

/*! @mainpage notitle
@section overview Plugin Overview

The Navigational Stars plugin marks the 58 navigational stars of the
Nautical Almanach and the 2102-D Rude Star Finder on the sky.

The NavStars class is the main class of the plug-in. It manages the list of
navigational stars and manipulate show/hide markers of them. All markers
are not objects!

The plugin is also an example of a custom plugin that just marks existing stars.

@section config Configuration
The plug-ins' configuration data is stored in Stellarium's main configuration
file (section [NavigationalStars]).
*/

//! @class NavStars
//! Main class of the %Navigational Stars plugin.
//! @author Alexander Wolf
class NavStars : public StelModule
{
	Q_OBJECT
	Q_PROPERTY(bool navStarsVisible
		   READ getNavStarsMarks
		   WRITE setNavStarsMarks
		   NOTIFY navStarsMarksChanged)
public:	
	NavStars();
	virtual ~NavStars();

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	virtual void init();
	virtual void deinit();
	virtual void update(double deltaTime);
	virtual void draw(StelCore* core);
	virtual double getCallOrder(StelModuleActionName actionName) const;


public slots:
	//! Set flag of displaying markers of the navigational stars
	//! Emits navStarsMarksChanged() if the value changes.
	void setNavStarsMarks(const bool b);
	//! Get flag of displaying markers of the navigational stars
	bool getNavStarsMarks(void) const;

signals:
	//! Emitted when display of markers have been changed.
	void navStarsMarksChanged(bool b);

private slots:
	//! Called setNavStarsMarks() if the value changes.
	void starNamesChanged(const bool b);

private:
	StarMgr* smgr;
	QSettings* conf;

	//! List of the navigational stars' HIP numbers.
	QList<int> starNumbers;
	//! List of pointers to the objects representing the stars.
	QVector<StelObjectP> stars;
	
	StelTextureSP markerTexture;
	//! Color used to paint each star's marker and additional label.
	Vec3f markerColor;	
	LinearFader markerFader;
	
	//! State of displaying stars labels.
	bool starNamesState;

	//! Button for the bottom toolbar.
	StelButton* toolbarButton;
};



#include <QObject>
#include "StelPluginInterface.hpp"

//! This class is used by Qt to manage a plug-in interface
class NavStarsStelPluginInterface : public QObject, public StelPluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID StelPluginInterface_iid)
	Q_INTERFACES(StelPluginInterface)
public:
	virtual StelModule* getStelModule() const;
	virtual StelPluginInfo getPluginInfo() const;
};

#endif // _NAVSTARS_HPP_
