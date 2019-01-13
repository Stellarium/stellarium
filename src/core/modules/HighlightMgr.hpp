/*
 * Copyright (C) 2018 Alexander Wolf
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

#ifndef HIGHTLIGHTMGR_HPP
#define HIGHTLIGHTMGR_HPP

#include "StelObjectModule.hpp"
#include "StelObject.hpp"
#include "StelTextureTypes.hpp"

#include <QList>

class StelPainter;
class QSettings;

class HighlightMgr : public StelObjectModule
{
	Q_OBJECT

public:
	HighlightMgr();
	virtual ~HighlightMgr();

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	virtual void init();
	virtual void deinit();
	virtual void update(double) {;}
	virtual void draw(StelCore* core);
	//virtual void drawPointer(StelCore* core, StelPainter& painter);
	virtual double getCallOrder(StelModuleActionName actionName) const;

	void drawHighlights(StelCore* core, StelPainter& painter);

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in StelObjectManager class
	//! Used to get a list of objects which are near to some position.
	//! @param v a vector representing the position in th sky around which to search for nebulae.
	//! @param limitFov the field of view around the position v in which to search for satellites.
	//! @param core the StelCore to use for computations.
	//! @return an list containing the satellites located inside the limitFov circle around position v.
	virtual QList<StelObjectP> searchAround(const Vec3d& v, double limitFov, const StelCore* core) const;

	//! Return the matching satellite object's pointer if exists or Q_NULLPTR.
	//! @param nameI18n The case in-sensistive satellite name
	virtual StelObjectP searchByNameI18n(const QString& nameI18n) const;

	//! Return the matching satellite if exists or Q_NULLPTR.
	//! @param name The case in-sensistive standard program name
	virtual StelObjectP searchByName(const QString& name) const;

	virtual StelObjectP searchByID(const QString &id) const;

	//! Find and return the list of at most maxNbItem objects auto-completing the passed object name.
	//! @param objPrefix the case insensitive first letters of the searched object
	//! @param maxNbItem the maximum number of returned object names
	//! @param useStartOfWords the autofill mode for returned objects names
	//! @return a list of matching object name by order of relevance, or an empty list if nothing match
	virtual QStringList listMatchingObjects(const QString& objPrefix, int maxNbItem=5, bool useStartOfWords=false, bool inEnglish=false) const;
	virtual QStringList listAllObjects(bool inEnglish) const;
	virtual QString getName() const { return "Highlights"; }
	virtual QString getStelObjectType() const { return QString(); }

	//! Fill the list highlight markers
	//! @param list - list of coordinates of the highlights (J2000.0 frame)
	void fillHighlightList(QList<Vec3d> list);

public slots:
	//! Set the color used to draw of the highlight markers.
	//! @param c The color of the highlight markers (R,G,B)
	//! @code
	//! // example of usage in scripts
	//! HighlightMgr.setColor(Vec3f(1.0,0.0,0.0));
	//! @endcode
	void setColor(const Vec3f& c);
	//! Get the current color used to draw of the highlight markers.
	//! @return current color
	const Vec3f& getColor(void) const;

	//! Set the size of highlight markers.
	//! @param c The size of the highlight markers
	//! @code
	//! // example of usage in scripts
	//! HighlightMgr.setMarkersSize(13.f);
	//! @endcode
	void setMarkersSize(const float size);
	//! Get the current size used to highlight markers.
	//! @return current size
	float getMarkersSize(void) const;

	//! Clean the list of highlight markers
	void cleanHighlightList();

	//! Add the point into list of the highlight markers
	//! @param ra - right ascension angle (J2000.0 frame) of highlight marker
	//! @param dec - declination angle (J2000.0 frame) of highlight marker
	//! @code
	//! // example of usage in scripts
	//! HighlightMgr.addPoint("2h10m15s", "60d01m15s");
	//! @endcode
	void addPoint(const QString& ra, const QString& dec);

	//! Add the point into list of the highlight markers
	//! @param ra - right ascension angle (on date) of highlight marker
	//! @param dec - declination angle (on date) of highlight marker
	//! @code
	//! // example of usage in scripts
	//! HighlightMgr.addPointRaDec("2h10m15s", "60d01m15s");
	//! @endcode
	void addPointRaDec(const QString& ra, const QString& dec);

	//! Add the point into list of the highlight markers
	//! @param alt - altitude of highlight marker
	//! @param azi - azimuth of highlight marker
	//! @code
	//! // example of usage in scripts
	//! HighlightMgr.addPointAltAzi("2d10m15s", "60d01m15s");
	//! @endcode
	void addPointAltAzi(const QString& alt, const QString& azi);

private:
	// Font used for displaying our text
	QSettings* conf;
	StelTextureSP texPointer;
	QList<Vec3d> highlightList;

	Vec3f hightlightColor;
	float markerSize;
};

#endif /* HIGHLIGHTMGR_HPP */
