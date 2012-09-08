/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau
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

#ifndef _STARMGR_HPP_
#define _STARMGR_HPP_

#include <QFont>
#include <QVariantMap>
#include "StelFader.hpp"
#include "StelObjectModule.hpp"
#include "StelProjectorType.hpp"

class StelObject;
class StelToneReproducer;
class StelProjector;
class QSettings;

namespace BigStarCatalogExtension {
  class ZoneArray;
  struct HipIndexStruct;
}

static const int RCMAG_TABLE_SIZE = 4096;

//! @class StarMgr
//! Stores the star catalogue data.
//! Used to render the stars themselves, as well as determine the color table
//! and render the labels of those stars with names for a given SkyCulture.
//!
//! The celestial sphere is split into zones, which correspond to the
//! triangular faces of a geodesic sphere. The number of zones (faces)
//! depends on the level of sub-division of this sphere. The lowest
//! level, 0, is an icosahedron (20 faces), subsequent levels, L,
//! of sub-division give the number of zones, n as:
//!
//! n=20 x 4^L
//!
//! Stellarium uses levels 0 to 7 in the existing star catalogues.
//! Star Data Records contain the position of a star as an offset from
//! the central position of the zone in which that star is located,
//! thus it is necessary to determine the vector from the observer
//! to the centre of a zone, and add the star's offsets to find the
//! absolute position of the star on the celestial sphere.
//!
//! This position for a star is expressed as a 3-dimensional vector
//! which points from the observer (at the centre of the geodesic sphere)
//! to the position of the star as observed on the celestial sphere.
class StarMgr : public StelObjectModule
{
	Q_OBJECT

public:
	StarMgr(void);
	~StarMgr(void);

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	//! Initialize the StarMgr.
	//! - Loads the star catalogue data into memory
	//! - Sets up the star color table
	//! - Loads the star texture
	//! - Loads the star font (for labels on named stars)
	//! - Loads the texture of the sar selection indicator
	//! - Lets various display flags from the ini parser object
	virtual void init();

	//! Draw the stars and the star selection indicator if necessary.
	virtual void draw(StelCore* core, class StelRenderer* renderer);

	//! Update any time-dependent features.
	//! Includes fading in and out stars and labels when they are turned on and off.
	virtual void update(double deltaTime) {labelsFader.update((int)(deltaTime*1000)); starsFader.update((int)(deltaTime*1000));}

	//! Used to determine the order in which the various StelModules are drawn.
	virtual double getCallOrder(StelModuleActionName actionName) const;

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in StelObjectManager class
	//! Return a list containing the stars located inside the limFov circle around position v
	virtual QList<StelObjectP > searchAround(const Vec3d& v, double limitFov, const StelCore* core) const;

	//! Return the matching Stars object's pointer if exists or NULL
	//! @param nameI18n The case in-sensistive star common name or HP
	//! catalog name (format can be HP1234 or HP 1234 or HIP 1234) or sci name
	virtual StelObjectP searchByNameI18n(const QString& nameI18n) const;

	//! Return the matching star if exists or NULL
	//! @param name The case in-sensistive standard program planet name
	virtual StelObjectP searchByName(const QString& name) const;

	//! Find and return the list of at most maxNbItem objects auto-completing the passed object I18n name.
	//! @param objPrefix the case insensitive first letters of the searched object
	//! @param maxNbItem the maximum number of returned object names
	//! @return a list of matching object name by order of relevance, or an empty list if nothing match
	virtual QStringList listMatchingObjectsI18n(const QString& objPrefix, int maxNbItem=5) const;
	// empty, as there's too much stars for displaying at once
	virtual QStringList listAllObjects(bool inEnglish) const { Q_UNUSED(inEnglish) return QStringList(); }
	virtual QString getName() const { return "Stars"; }

public slots:
	///////////////////////////////////////////////////////////////////////////
	// Methods callable from script and GUI
	//! Set the color used to label bright stars.
	void setLabelColor(const Vec3f& c) {labelColor = c;}
	//! Get the current color used to label bright stars.
	Vec3f getLabelColor(void) const {return labelColor;}

	//! Set display flag for Stars.
	void setFlagStars(bool b) {starsFader=b;}
	//! Get display flag for Stars
	bool getFlagStars(void) const {return starsFader==true;}

	//! Set display flag for Star names (labels).
	void setFlagLabels(bool b) {labelsFader=b;}
	//! Get display flag for Star names (labels).
	bool getFlagLabels(void) const {return labelsFader==true;}

	//! Set the amount of star labels. The real amount is also proportional with FOV.
	//! The limit is set in function of the stars magnitude
	//! @param a the amount between 0 and 10. 0 is no labels, 10 is maximum of labels
	void setLabelsAmount(float a) {labelsAmount=a;}
	//! Get the amount of star labels. The real amount is also proportional with FOV.
	//! @return the amount between 0 and 10. 0 is no labels, 10 is maximum of labels
	float getLabelsAmount(void) const {return labelsAmount;}

	//! Define font size to use for star names display.
	void setFontSize(double newFontSize);

	//! Show scientific or catalog names on stars without common names.
	static void setFlagSciNames(bool f) {flagSciNames = f;}
	static bool getFlagSciNames(void) {return flagSciNames;}

public:
	///////////////////////////////////////////////////////////////////////////
	// Other methods
	//! Search by Hipparcos catalogue number.
	//! @param hip the Hipparcos catalogue number of the star which is required.
	//! @return the requested StelObjectP or an empty objecy if the requested
	//! one was not found.
	StelObjectP searchHP(int hip) const;

	//! Get the (translated) common name for a star with a specified
	//! Hipparcos catalogue number.
	static QString getCommonName(int hip);

	//! Get the (translated) scientific name for a star with a specified
	//! Hipparcos catalogue number.
	static QString getSciName(int hip);

	static QString convertToSpectralType(int index);
	static QString convertToComponentIds(int index);

	QVariantList getCatalogsDescription() const {return catalogsDescription;}

	//! Try to load the given catalog, even if it is marched as unchecked.
	//! Mark it as checked if checksum is correct.
	//! @return false in case of failure.
	bool checkAndLoadCatalog(QVariantMap m);

private slots:
	void setStelStyle(const QString& section);
	//! Translate text.
	void updateI18n();

	//! Called when the sky culture is updated.
	//! Loads common and scientific names of stars for a given sky culture.
	//! @param skyCultureDir the name of the directory containing the sky culture to use.
	void updateSkyCulture(const QString& skyCultureDir);

private:

	void setCheckFlag(const QString& catalogId, bool b);

	void copyDefaultConfigFile();

	//! Loads common names for stars from a file.
	//! Called when the SkyCulture is updated.
	//! @param the path to a file containing the common names for bright stars.
	int loadCommonNames(const QString& commonNameFile);

	//! Loads scientific names for stars from a file.
	//! Called when the SkyCulture is updated.
	//! @param the path to a file containing the scientific names for bright stars.
	void loadSciNames(const QString& sciNameFile);

	//! Gets the maximum search level.
	// TODO: add a non-lame description - what is the purpose of the max search level?
	int getMaxSearchLevel() const;

	//! Load all the stars from the files.
	void loadData(QVariantMap starsConfigFile);

	//! Draw a nice animated pointer around the object.
	void drawPointer(class StelRenderer* renderer, StelProjectorP projector, const StelCore* core);

	LinearFader labelsFader;
	LinearFader starsFader;

	bool flagStarName;
	float labelsAmount;
	bool gravityLabel;

	int maxGeodesicGridLevel;
	int lastMaxSearchLevel;
	typedef QHash<int,BigStarCatalogExtension::ZoneArray*> ZoneArrayMap;
	ZoneArrayMap zoneArrays; // index is the grid level
	static void initTriangleFunc(int lev, int index,
								 const Vec3f &c0,
								 const Vec3f &c1,
								 const Vec3f &c2,
								 void *context)
	{
		reinterpret_cast<StarMgr*>(context)->initTriangle(lev, index, c0, c1, c2);
	}

	void initTriangle(int lev, int index,
					  const Vec3f &c0,
					  const Vec3f &c1,
					  const Vec3f &c2);

	BigStarCatalogExtension::HipIndexStruct *hipIndex; // array of hiparcos stars

	static QHash<int, QString> commonNamesMap;
	static QHash<int, QString> commonNamesMapI18n;
	static QMap<QString, int> commonNamesIndexI18n;

	static QHash<int, QString> sciNamesMapI18n;
	static QMap<QString, int> sciNamesIndexI18n;

	QFont starFont;
	static bool flagSciNames;
	Vec3f labelColor;

	class StelTextureNew* texPointer;		// The selection pointer texture

	class StelObjectMgr* objectMgr;

	QString starConfigFileFullPath;
	QVariantMap starSettings;
	QVariantList catalogsDescription;
};


#endif // _STARMGR_HPP_

