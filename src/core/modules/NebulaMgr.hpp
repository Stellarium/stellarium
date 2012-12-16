/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau
 * Copyright (C) 2011 Alexander Wolf
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

#ifndef _NEBULAMGR_HPP_
#define _NEBULAMGR_HPP_

#include <QString>
#include <QStringList>
#include <QFont>
#include "Nebula.hpp"
#include "StelObjectType.hpp"
#include "StelFader.hpp"
#include "StelSphericalIndex.hpp"
#include "StelObjectModule.hpp"

class Nebula;
class StelTranslator;
class StelToneReproducer;
class QSettings;

typedef QSharedPointer<Nebula> NebulaP;

//! @class NebulaMgr
//! Manage a collection of nebulae. This class is used
//! to display the NGC catalog with information, and textures for some of them.
class NebulaMgr : public StelObjectModule
{
	Q_OBJECT

public:
	NebulaMgr();
	virtual ~NebulaMgr();

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	//! Initialize the NebulaMgr object.
	//!  - Load the font into the Nebula class, which is used to draw Nebula labels.
	//!  - Load the texture used to draw nebula locations into the Nebula class (for
	//!     those with no individual texture).
	//!  - Load the pointer texture.
	//!  - Set flags values from ini parser which relate to nebula display.
	//!  - call updateI18n() to translate names.
	virtual void init();

	//! Draws all nebula objects.
	virtual void draw(StelCore* core, class StelRenderer* renderer);

	//! Update state which is time dependent.
	virtual void update(double deltaTime) {hintsFader.update((int)(deltaTime*1000)); flagShow.update((int)(deltaTime*1000));}

	//! Determines the order in which the various modules are drawn.
	virtual double getCallOrder(StelModuleActionName actionName) const;

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in StelObjectManager class
	//! Used to get a vector of objects which are near to some position.
	//! @param v a vector representing the position in th sky around which to search for nebulae.
	//! @param limitFov the field of view around the position v in which to search for nebulae.
	//! @param core the StelCore to use for computations.
	//! @return an list containing the nebulae located inside the limitFov circle around position v.
	virtual QList<StelObjectP> searchAround(const Vec3d& v, double limitFov, const StelCore* core) const;

	//! Return the matching nebula object's pointer if exists or NULL.
	//! @param nameI18n The case in-sensistive nebula name or NGC M catalog name : format can
	//! be M31, M 31, NGC31, NGC 31
	virtual StelObjectP searchByNameI18n(const QString& nameI18n) const;

	//! Return the matching nebula if exists or NULL.
	//! @param name The case in-sensistive standard program name
	virtual StelObjectP searchByName(const QString& name) const;

	//! Find and return the list of at most maxNbItem objects auto-completing the passed object I18n name.
	//! @param objPrefix the case insensitive first letters of the searched object
	//! @param maxNbItem the maximum number of returned object names
	//! @return a list of matching object name by order of relevance, or an empty list if nothing match
	virtual QStringList listMatchingObjectsI18n(const QString& objPrefix, int maxNbItem=5) const;
	// empty for now
	virtual QStringList listAllObjects(bool inEnglish) const { Q_UNUSED(inEnglish) return QStringList(); }
	virtual QString getName() const { return "Nebulae"; }

	///////////////////////////////////////////////////////////////////////////
	// Properties setters and getters
public slots:
	//! Set the color used to draw the nebula circles.
	void setCirclesColor(const Vec3f& c);
	//! Get current value of the nebula circle color.
	const Vec3f& getCirclesColor(void) const;

	//! Set Nebulae Hints circle scale.
	void setCircleScale(float scale);
	//! Get Nebulae Hints circle scale.
	float getCircleScale(void) const;

	//! Set how long it takes for nebula hints to fade in and out when turned on and off.
	void setHintsFadeDuration(float duration) {hintsFader.setDuration((int) (duration * 1000.f));}

	//! Set flag for displaying Nebulae Hints.
	void setFlagHints(bool b) {hintsFader=b;}
	//! Get flag for displaying Nebulae Hints.
	bool getFlagHints(void) const {return hintsFader;}

	//! Set flag used to turn on and off Nebula rendering.
	void setFlagShow(bool b) { flagShow = b; }
	//! Get value of flag used to turn on and off Nebula rendering.
	bool getFlagShow(void) const { return flagShow; }

	//! Set the color used to draw nebula labels.
	void setLabelsColor(const Vec3f& c);
	//! Get current value of the nebula label color.
	const Vec3f& getLabelsColor(void) const;

	//! Set the amount of nebulae labels. The real amount is also proportional with FOV.
	//! The limit is set in function of the nebulae magnitude
	//! @param a the amount between 0 and 10. 0 is no labels, 10 is maximum of labels
	void setLabelsAmount(float a) {labelsAmount=a;}
	//! Get the amount of nebulae labels. The real amount is also proportional with FOV.
	//! @return the amount between 0 and 10. 0 is no labels, 10 is maximum of labels
	float getLabelsAmount(void) const {return labelsAmount;}

	//! Set the amount of nebulae hints. The real amount is also proportional with FOV.
	//! The limit is set in function of the nebulae magnitude
	//! @param f the amount between 0 and 10. 0 is no hints, 10 is maximum of hints
	void setHintsAmount(float f) {hintsAmount = f;}
	//! Get the amount of nebulae labels. The real amount is also proportional with FOV.
	//! @return the amount between 0 and 10. 0 is no hints, 10 is maximum of hints
	float getHintsAmount(void) const {return hintsAmount;}

private slots:
	//! Sets the colors of the Nebula labels and markers according to the
	//! values in a configuration object
	void setStelStyle(const QString& section);

	//! Update i18 names from English names according to passed translator.
	//! The translation is done using gettext with translated strings defined
	//! in translations.h
	void updateI18n();
	

private:
	//! Search for a nebula object by name. e.g. M83, NGC 1123, IC 1234.
	NebulaP search(const QString& name);

	//! Search the Nebulae by position
	NebulaP search(const Vec3d& pos);

	//! Load a set of nebula images.
	//! Each sub-directory of the INSTALLDIR/nebulae directory contains a set of
	//! nebula textures.  The sub-directory is the setName.  Each set has its
	//! own nebula_textures.fab file and corresponding image files.
	//! This function loads a set of textures.
	//! @param setName a string which corresponds to the directory where the set resides
	void loadNebulaSet(const QString& setName);

	//! Draw a nice animated pointer around the object
	void drawPointer(const StelCore* core, class StelRenderer* renderer);

	NebulaP searchM(unsigned int M);
	NebulaP searchNGC(unsigned int NGC);
	NebulaP searchIC(unsigned int IC);
	bool loadNGC(const QString& fileName);
	bool loadNGCOld(const QString& catNGC);
	bool loadNGCNames(const QString& fileName);

	QVector<NebulaP> nebArray;		// The nebulas list
	QHash<unsigned int, NebulaP> ngcIndex;
	LinearFader hintsFader;
	LinearFader flagShow;

	//! The internal grid for fast positional lookup
	StelSphericalIndex nebGrid;

	//! The amount of hints (between 0 and 10)
	float hintsAmount;
	//! The amount of labels (between 0 and 10)
	float labelsAmount;

	//! The selection pointer texture
	StelTextureNew* texPointer;
	
	QFont nebulaFont;      // Font used for names printing

	//! Textures used to draw nebula hints.
	Nebula::NebulaHintTextures nebulaHintTextures;
};

#endif // _NEBULAMGR_HPP_
