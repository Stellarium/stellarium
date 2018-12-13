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

public slots:
	//! Set the color used to draw of the highlight markers.
	//! @param c The color of the highlight markers (R,G,B)
	//! @code
	//! // example of usage in scripts
	//! HighlightMgr.setHighlightColor(Vec3f(1.0,0.0,0.0));
	//! @endcode
	void setHighlightColor(const Vec3f& c);
	//! Get the current color used to draw of the highlight markers.
	//! @return current color
	const Vec3f& getHighlightColor(void) const;

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

	//! Fill the list highlight markers
	//! @param list - list of coordinates of the highlights
	void fillHighlightList(QList<Vec3d> list);

	//! Clean the list of highlight markers
	void cleanHighlightList();


private:
	// Font used for displaying our text
	QSettings* conf;
	StelTextureSP texPointer;
	QList<Vec3d> highlightList;

	Vec3f hightlightColor;
	float markerSize;
};

#endif /* HIGHLIGHTMGR_HPP */
