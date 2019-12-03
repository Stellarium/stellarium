/*
 * Stellarium
 * Copyright (C) 2019 Alexander Wolf
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef MARKERMGR_HPP
#define MARKERMGR_HPP

#include "StelModule.hpp"

#include <QMap>
#include <QString>

class StelCore;
class StelPainter;

//! @class MarkerMgr
//! Allows for creation of custom markers on objects or coordinates.
//! Because this class is intended for use in scripting (although other uses are also fine), all marker
//! types and so on are specified by QString descriptions.
//! The markers are painted very late, i.e. also sky object markers will be written over the landscape.
class MarkerMgr : public StelModule
{
	Q_OBJECT

public:
	//! Construct a MarkerMgr object.
	MarkerMgr();
	virtual ~MarkerMgr();
 
	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	//! Initialize the MarkerMgr object.
	virtual void init();
	
	//! Draw user labels.
	virtual void draw(StelCore* core);
	
	//! Update time-dependent parts of the module.
	virtual void update(double deltaTime);

	//! Defines the order in which the various modules are drawn.
	virtual double getCallOrder(StelModuleActionName actionName) const;

public slots:
	//! Create a marker which is attached to a StelObject.
	//! @param objectName the English name of the object to attach to
	//! @param visible if true, the marker starts displayed, else it starts hidden
	//! @param mtype the type of marker. Keys:
	//! - cross: simple cross
	//! - circle: simple circle
	//! - ellipse: simple horizontal ellipse
	//! - square: simple square
	//! - dotted-circle: simple circle drawn by dots
	//! - circled-cross (or circled-plus): simple circle with cross inside the circle
	//! - dashed-square: simple square, drawing by dashed line
	//! - squared-dotted-circle (or squared-dcircle): simple circle, drawing by dotted line
	//! - crossed-circle: the cross under circle
	//! - target: the target shape
	//! - gear: the gear shape
	//! - disk: the disk shape
	//! @param color HTML-like color spec, e.g. "#ffff00" for yellow
	//! @param size the size of marker.
	//! @param autoDelete the marker will be automatically deleted after it is displayed once
	//! @param autoDeleteTimeoutMs if not zero, the marker will be automatically deleted after
	//! autoDeleteTimeoutMs ms
	//! @return a unique ID which can be used to refer to the marker.
	//! returns -1 if the marker could not be created (e.g. object not found)
	int markerObject(const QString& objectName,
			 bool visible=true,
			 const QString& mtype="cross",
			 const QString& color="#ffff66",
			 const float size = 6.f,
			 bool autoDelete = false,
			 int autoDeleteTimeoutMs = 0);

	//! Create a marker with equatorial coordinates.
	//! @param RA right ascension (e.g. 5h10m31s)
	//! @param Dec declination (e.g. 25d30m30s)
	//! @param j2000epoch if true, the marker starts displayed in equatorial coordinates for epoch J2000.0
	//! @param visible if true, the marker starts displayed, else it starts hidden
	//! @param mtype the type of marker. Keys:
	//! - cross: simple cross
	//! - circle: simple circle
	//! - ellipse: simple horizontal ellipse
	//! - square: simple square
	//! - dotted-circle: simple circle drawn by dots
	//! - circled-cross (or circled-plus): simple circle with cross inside the circle
	//! - dashed-square: simple square, drawing by dashed line
	//! - squared-dotted-circle (or squared-dcircle): simple circle, drawing by dotted line
	//! - crossed-circle: the cross under circle
	//! - target: the target shape
	//! - gear: the gear shape
	//! - disk: the disk shape
	//! @param color HTML-like color spec, e.g. "#ffff00" for yellow
	//! @param size the size of marker.
	//! @param autoDelete the marker will be automatically deleted after it is displayed once
	//! @param autoDeleteTimeoutMs if not zero, the marker will be automatically deleted after
	//! autoDeleteTimeoutMs ms
	//! @return a unique ID which can be used to refer to the marker.
	//! returns -1 if the marker could not be created (e.g. object not found)
	int markerEquatorial(const QString& RA,
			     const QString& Dec,
			     bool j2000epoch=true,
			     bool visible=true,
			     const QString& mtype="cross",
			     const QString& color="#ffff66",
			     const float size = 6.f,
			     bool autoDelete = false,
			     int autoDeleteTimeoutMs = 0);

	//! Create a marker with horizontal coordinates.
	//! @param az azimuth (e.g. 125d30m30s)
	//! @param alt altitude (e.g. 25d30m30s)
	//! @param visible if true, the marker starts displayed, else it starts hidden
	//! @param mtype the type of marker. Keys:
	//! - cross: simple cross
	//! - circle: simple circle
	//! - ellipse: simple horizontal ellipse
	//! - square: simple square
	//! - dotted-circle: simple circle drawn by dots
	//! - circled-cross (or circled-plus): simple circle with cross inside the circle
	//! - dashed-square: simple square, drawing by dashed line
	//! - squared-dotted-circle (or squared-dcircle): simple circle, drawing by dotted line
	//! - crossed-circle: the cross under circle
	//! - target: the target shape
	//! - gear: the gear shape
	//! - disk: the disk shape
	//! @param color HTML-like color spec, e.g. "#ffff00" for yellow
	//! @param size the size of marker.
	//! @param autoDelete the marker will be automatically deleted after it is displayed once
	//! @param autoDeleteTimeoutMs if not zero, the marker will be automatically deleted after
	//! autoDeleteTimeoutMs ms
	//! @return a unique ID which can be used to refer to the marker.
	//! returns -1 if the marker could not be created (e.g. object not found)
	int markerHorizon(const QString& az,
			  const QString& alt,
			  bool visible=true,
			  const QString& mtype="cross",
			  const QString& color="#ffff66",
			  const float size = 6.f,
			  bool autoDelete = false,
			  int autoDeleteTimeoutMs = 0);

	//! find out if a marker identified by id is presently shown
	bool getMarkerShow(int id) const;
	//! set a marker identified by id to be shown or not
	void setMarkerShow(int id, bool show);
	//! Delete a marker by the ID
	//! @return true if the id existed and was deleted, else false
	void deleteMarker(int id);
	//! Delete all markers.
	//! @return the number of markers deleted
	int deleteAllMarkers(void);

private slots:
	void markerVisibleTimeout();
	void markerDeleteTimeout();

private:
	QMap<int, class StelMarker*> allMarkers;
	int counter;
	int appendMarker(class StelMarker* m, int autoDeleteTimeoutMs);
};

#endif // MARKERMGR_HPP
