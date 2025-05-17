/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau
 * Copyright (C) 2011 Alexander Wolf
 * Copyright (C) 2015 Georg Zotti
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

#ifndef NEBULAMGR_HPP
#define NEBULAMGR_HPP

#include "StelObjectType.hpp"
#include "StelFader.hpp"
#include "StelSphericalIndex.hpp"
#include "StelObjectModule.hpp"
#include "StelTextureTypes.hpp"
#include "Nebula.hpp"

#include <QString>
#include <QStringList>
#include <QFont>

class StelTranslator;
class StelSkyCulture;
class StelToneReproducer;
class QSettings;
class StelPainter;

typedef QSharedPointer<Nebula> NebulaP;

//! @class NebulaMgr
//! Manage a collection of nebulae. This class is used
//! to display the M, NGC, IC, Barnard, and many other catalogs with information, and textures for some of them.
// GZ: This doc seems widely outdated/misleading - photo textures are not managed here but in StelSkyImageTile

//! Stellarium is prepared to manage more than one set of nebulae (Deep-Sky Objects).
//! Currently all data are loaded from directory nebulae/default/.
//! The actual textures are managed in a StelSphericalIndex.
//! The file names.dat is an annotated list of object nicknames, providing names and references.
//! Some names are a bit too fancyful for some users, so references can be excluded from loading
//! by setting an entry in config.ini:
//! [astro]
//! nebula_exclude_references=default:ADI,DSC-HT;custom:foo,bar
//! This would exclude references ADI and DSC-HT from nebulae/default/names.dat
//! and references foo and bar from a (future) nebulae/custom/names.dat nebula set.
//! If the value string does not contain a colon, it is assumed we mean default.

class NebulaMgr : public StelObjectModule
{
	Q_OBJECT
	//StelActions
	Q_PROPERTY(bool flagHintDisplayed
		   READ getFlagHints
		   WRITE setFlagHints
		   NOTIFY flagHintsDisplayedChanged)
	Q_PROPERTY(bool flagTypeFiltersUsage
		   READ getFlagUseTypeFilters
		   WRITE setFlagUseTypeFilters
		   NOTIFY flagUseTypeFiltersChanged)
	//StelProperties
	// This used to be of type Nebula::TypeGroup, however on Qt6 this does not work and was changed to int.
	Q_PROPERTY(int typeFilters
		   READ getTypeFilters
		   WRITE setTypeFilters
		   NOTIFY typeFiltersChanged
		   )
	// This used to be of type Nebula::CatalogGroup, however on Qt6 this does not work and was changed to int.
	Q_PROPERTY(int catalogFilters
		   READ getCatalogFilters
		   WRITE setCatalogFilters
		   NOTIFY catalogFiltersChanged
		   )
	Q_PROPERTY(bool hintsProportional
		   READ getHintsProportional
		   WRITE setHintsProportional
		   NOTIFY hintsProportionalChanged
		   )
	Q_PROPERTY(bool flagOutlinesDisplayed
		   READ getFlagOutlines
		   WRITE setFlagOutlines
		   NOTIFY flagOutlinesDisplayedChanged
		   )
	Q_PROPERTY(bool flagAdditionalNamesDisplayed
		   READ getFlagAdditionalNames
		   WRITE setFlagAdditionalNames
		   NOTIFY flagAdditionalNamesDisplayedChanged
		   )
	Q_PROPERTY(bool flagSurfaceBrightnessUsage
		   READ getFlagSurfaceBrightnessUsage
		   WRITE setFlagSurfaceBrightnessUsage
		   NOTIFY flagSurfaceBrightnessUsageChanged
		   )
	Q_PROPERTY(bool flagSurfaceBrightnessArcsecUsage
		   READ getFlagSurfaceBrightnessArcsecUsage
		   WRITE setFlagSurfaceBrightnessArcsecUsage
		   NOTIFY flagSurfaceBrightnessArcsecUsageChanged
		   )
	Q_PROPERTY(bool flagSurfaceBrightnessShortNotationUsage
		   READ getFlagSurfaceBrightnessShortNotationUsage
		   WRITE setFlagSurfaceBrightnessShortNotationUsage
		   NOTIFY flagSurfaceBrightnessShortNotationUsageChanged
		   )
	Q_PROPERTY(double labelsAmount
		   READ getLabelsAmount
		   WRITE setLabelsAmount
		   NOTIFY labelsAmountChanged
		   )
	Q_PROPERTY(double hintsAmount
		   READ getHintsAmount
		   WRITE setHintsAmount
		   NOTIFY hintsAmountChanged
		   )
	Q_PROPERTY(bool flagDesignationLabels
		   READ getDesignationUsage
		   WRITE setDesignationUsage
		   NOTIFY designationUsageChanged
		   )
	Q_PROPERTY(bool flagShowOnlyNamedDSO
		   READ getFlagShowOnlyNamedDSO
		   WRITE setFlagShowOnlyNamedDSO
		   NOTIFY flagShowOnlyNamedDSOChanged
		   )
	Q_PROPERTY(bool flagUseSizeLimits
		   READ getFlagSizeLimitsUsage
		   WRITE setFlagSizeLimitsUsage
		   NOTIFY flagSizeLimitsUsageChanged
		   )
	Q_PROPERTY(double minSizeLimit
		   READ getMinSizeLimit
		   WRITE setMinSizeLimit
		   NOTIFY minSizeLimitChanged
		   )
	Q_PROPERTY(double maxSizeLimit
		   READ getMaxSizeLimit
		   WRITE setMaxSizeLimit
		   NOTIFY maxSizeLimitChanged
		   )
	// Colors
	Q_PROPERTY(double hintsBrightness
		   READ getHintsBrightness
		   WRITE setHintsBrightness
		   NOTIFY hintsBrightnessChanged
		   )
	Q_PROPERTY(double labelsBrightness
		   READ getLabelsBrightness
		   WRITE setLabelsBrightness
		   NOTIFY labelsBrightnessChanged
		   )
	Q_PROPERTY(Vec3f labelsColor
		   READ getLabelsColor
		   WRITE setLabelsColor
		   NOTIFY labelsColorChanged
		   )
	Q_PROPERTY(Vec3f circlesColor
		   READ getCirclesColor
		   WRITE setCirclesColor
		   NOTIFY circlesColorChanged
		   )
	Q_PROPERTY(Vec3f regionsColor
		   READ getRegionsColor
		   WRITE setRegionsColor
		   NOTIFY regionsColorChanged
		   )
	Q_PROPERTY(Vec3f galaxiesColor
		   READ getGalaxyColor
		   WRITE setGalaxyColor
		   NOTIFY galaxiesColorChanged
		   )
	Q_PROPERTY(Vec3f activeGalaxiesColor
		   READ getActiveGalaxyColor
		   WRITE setActiveGalaxyColor
		   NOTIFY activeGalaxiesColorChanged
		   )
	Q_PROPERTY(Vec3f radioGalaxiesColor
		   READ getRadioGalaxyColor
		   WRITE setRadioGalaxyColor
		   NOTIFY radioGalaxiesColorChanged
		   )
	Q_PROPERTY(Vec3f interactingGalaxiesColor
		   READ getInteractingGalaxyColor
		   WRITE setInteractingGalaxyColor
		   NOTIFY interactingGalaxiesColorChanged
		   )
	Q_PROPERTY(Vec3f quasarsColor
		   READ getQuasarColor
		   WRITE setQuasarColor
		   NOTIFY quasarsColorChanged
		   )
	Q_PROPERTY(Vec3f possibleQuasarsColor
		   READ getPossibleQuasarColor
		   WRITE setPossibleQuasarColor
		   NOTIFY possibleQuasarsColorChanged
		   )
	Q_PROPERTY(Vec3f clustersColor
		   READ getClusterColor
		   WRITE setClusterColor
		   NOTIFY clustersColorChanged
		   )
	Q_PROPERTY(Vec3f openClustersColor
		   READ getOpenClusterColor
		   WRITE setOpenClusterColor
		   NOTIFY openClustersColorChanged
		   )
	Q_PROPERTY(Vec3f globularClustersColor
		   READ getGlobularClusterColor
		   WRITE setGlobularClusterColor
		   NOTIFY globularClustersColorChanged
		   )
	Q_PROPERTY(Vec3f stellarAssociationsColor
		   READ getStellarAssociationColor
		   WRITE setStellarAssociationColor
		   NOTIFY stellarAssociationsColorChanged
		   )
	Q_PROPERTY(Vec3f starCloudsColor
		   READ getStarCloudColor
		   WRITE setStarCloudColor
		   NOTIFY starCloudsColorChanged
		   )
	Q_PROPERTY(Vec3f starsColor
		   READ getStarColor
		   WRITE setStarColor
		   NOTIFY starsColorChanged
		   )
	Q_PROPERTY(Vec3f symbioticStarsColor
		   READ getSymbioticStarColor
		   WRITE setSymbioticStarColor
		   NOTIFY symbioticStarsColorChanged
		   )
	Q_PROPERTY(Vec3f emissionLineStarsColor
		   READ getEmissionLineStarColor
		   WRITE setEmissionLineStarColor
		   NOTIFY emissionLineStarsColorChanged
		   )
	Q_PROPERTY(Vec3f nebulaeColor
		   READ getNebulaColor
		   WRITE setNebulaColor
		   NOTIFY nebulaeColorChanged
		   )
	Q_PROPERTY(Vec3f planetaryNebulaeColor
		   READ getPlanetaryNebulaColor
		   WRITE setPlanetaryNebulaColor
		   NOTIFY planetaryNebulaeColorChanged
		   )
	Q_PROPERTY(Vec3f darkNebulaeColor
		   READ getDarkNebulaColor
		   WRITE setDarkNebulaColor
		   NOTIFY darkNebulaeColorChanged
		   )
	Q_PROPERTY(Vec3f reflectionNebulaeColor
		   READ getReflectionNebulaColor
		   WRITE setReflectionNebulaColor
		   NOTIFY reflectionNebulaeColorChanged
		   )
	Q_PROPERTY(Vec3f bipolarNebulaeColor
		   READ getBipolarNebulaColor
		   WRITE setBipolarNebulaColor
		   NOTIFY bipolarNebulaeColorChanged
		   )
	Q_PROPERTY(Vec3f emissionNebulaeColor
		   READ getEmissionNebulaColor
		   WRITE setEmissionNebulaColor
		   NOTIFY emissionNebulaeColorChanged
		   )
	Q_PROPERTY(Vec3f possiblePlanetaryNebulaeColor
		   READ getPossiblePlanetaryNebulaColor
		   WRITE setPossiblePlanetaryNebulaColor
		   NOTIFY possiblePlanetaryNebulaeColorChanged
		   )
	Q_PROPERTY(Vec3f protoplanetaryNebulaeColor
		   READ getProtoplanetaryNebulaColor
		   WRITE setProtoplanetaryNebulaColor
		   NOTIFY protoplanetaryNebulaeColorChanged
		   )
	Q_PROPERTY(Vec3f hydrogenRegionsColor
		   READ getHydrogenRegionColor
		   WRITE setHydrogenRegionColor
		   NOTIFY hydrogenRegionsColorChanged
		   )
	Q_PROPERTY(Vec3f interstellarMatterColor
		   READ getInterstellarMatterColor
		   WRITE setInterstellarMatterColor
		   NOTIFY interstellarMatterColorChanged
		   )
	Q_PROPERTY(Vec3f emissionObjectsColor
		   READ getEmissionObjectColor
		   WRITE setEmissionObjectColor
		   NOTIFY emissionObjectsColorChanged
		   )
	Q_PROPERTY(Vec3f molecularCloudsColor
		   READ getMolecularCloudColor
		   WRITE setMolecularCloudColor
		   NOTIFY molecularCloudsColorChanged
		   )
	Q_PROPERTY(Vec3f blLacObjectsColor
		   READ getBlLacObjectColor
		   WRITE setBlLacObjectColor
		   NOTIFY blLacObjectsColorChanged
		   )
	Q_PROPERTY(Vec3f blazarsColor
		   READ getBlazarColor
		   WRITE setBlazarColor
		   NOTIFY blazarsColorChanged
		   )
	Q_PROPERTY(Vec3f youngStellarObjectsColor
		   READ getYoungStellarObjectColor
		   WRITE setYoungStellarObjectColor
		   NOTIFY youngStellarObjectsColorChanged
		   )
	Q_PROPERTY(Vec3f supernovaRemnantsColor
		   READ getSupernovaRemnantColor
		   WRITE setSupernovaRemnantColor
		   NOTIFY supernovaRemnantsColorChanged
		   )
	Q_PROPERTY(Vec3f supernovaCandidatesColor
		   READ getSupernovaCandidateColor
		   WRITE setSupernovaCandidateColor
		   NOTIFY supernovaCandidatesColorChanged
		   )
	Q_PROPERTY(Vec3f supernovaRemnantCandidatesColor
		   READ getSupernovaRemnantCandidateColor
		   WRITE setSupernovaRemnantCandidateColor
		   NOTIFY supernovaRemnantCandidatesColorChanged
		   )
	Q_PROPERTY(Vec3f galaxyClustersColor
		   READ getGalaxyClusterColor
		   WRITE setGalaxyClusterColor
		   NOTIFY galaxyClustersColorChanged
		   )

public:
	NebulaMgr();
	~NebulaMgr() override;

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	//! Initialize the NebulaMgr object.
	//!  - Load the font into the Nebula class, which is used to draw Nebula labels.
	//!  - Load the texture used to draw nebula locations into the Nebula class (for
	//!     those with no individual texture).
	//!  - Load the pointer texture.
	//!  - Set flags values from ini parser which relate to nebula display.
	//!  - call updateI18n() to translate names.
	void init() override;

	//! Draws all nebula objects and the pointer.
	void draw(StelCore* core) override;

	//! Update state which is time dependent.
	void update(double deltaTime) override {hintsFader.update(static_cast<int>(deltaTime*1000)); flagShow.update(static_cast<int>(deltaTime*1000));}

	//! Determines the order in which the various modules are drawn.
	double getCallOrder(StelModuleActionName actionName) const override;

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in StelObjectModule class
	//! Used to get a vector of objects which are near to some position.
	//! @param v a vector representing the position in th sky around which to search for nebulae.
	//! @param limitFov the field of view around the position v in which to search for nebulae.
	//! @param core the StelCore to use for computations.
	//! @return a list containing the nebulae located inside the limitFov circle around position v.
	QList<StelObjectP> searchAround(const Vec3d& v, double limitFov, const StelCore* core) const override;

	//! Return the matching nebula object's pointer if exists or an "empty" StelObjectP.
	//! @param nameI18n The case in-sensitive nebula name or NGC M catalog name : format can
	//! be M31, M 31, NGC31, NGC 31
	StelObjectP searchByNameI18n(const QString& nameI18n) const override;

	//! Return the matching nebula if exists or Q_NULLPTR.
	//! @param name The case in-sensitive standard program name
	StelObjectP searchByName(const QString& name) const override;

	StelObjectP searchByID(const QString &id) const override { return searchByName(id); }

	//! Find and return the list of at most maxNbItem objects auto-completing the passed object English name.
	//! @param objPrefix the case insensitive first letters of the searched object
	//! @param maxNbItem the maximum number of returned object names
	//! @param useStartOfWords the autofill mode for returned objects names
	//! @return a list of matching object name by order of relevance, or an empty list if nothing match
	QStringList listMatchingObjects(const QString& objPrefix, int maxNbItem=5, bool useStartOfWords=false) const override;
	//! @note Loading deep-sky objects with the proper names only.
	QStringList listAllObjects(bool inEnglish) const override;
	QStringList listAllObjectsByType(const QString& objType, bool inEnglish) const override;
	QString getName() const override { return "Deep-sky objects"; }
	QString getStelObjectType() const override { return Nebula::NEBULA_TYPE; }

	//! Compute the maximum magntiude for which hints will be displayed.
	float computeMaxMagHint(const class StelSkyDrawer* skyDrawer) const;

	//! Get designation for latest selected DSO with priority
	//! @note using for bookmarks feature as example
	//! @return a designation
	QString getLatestSelectedDSODesignation() const;
	//! Get designation for latest selected DSO with priority and ignorance of availability of catalogs
	//! @note using for bookmarks feature as example
	//! @return a designation
	QString getLatestSelectedDSODesignationWIC() const;

	//! Get the list of all deep-sky objects.
	const QVector<NebulaP>& getAllDeepSkyObjects() const { return dsoArray; }

	//! Get the list of deep-sky objects by type.
	const QList<NebulaP> getDeepSkyObjectsByType(const QString& objType) const;

	///////////////////////////////////////////////////////////////////////////
	// Properties setters and getters
public slots:
	void setCatalogFilters(int cflags);
	int getCatalogFilters() const { return int(Nebula::catalogFilters); }
	//! Activate all catalogs
	void selectAllCatalogs();
	//! Activate a useful selection of catalogs: M, NGC, IC
	void selectStandardCatalogs();
	//! Disable all catalogs
	void selectNoneCatalogs();
	//! retrieve configured catalogs from config.ini.
	void loadCatalogFilters();
	//! store configured catalogs into config.ini.
	void storeCatalogFilters();

	void setTypeFilters(int tflags);
	int getTypeFilters() const { return int(Nebula::typeFilters); }

	//! Set the default color used to draw the nebula symbols (default circles, etc).
	//! @param c The color of the nebula symbols
	//! @code
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(0.6, 0.8, 0.0);
	//! NebulaMgr.setCirclesColor(c.toVec3f());
	//! @endcode
	void setCirclesColor(const Vec3f& c);
	//! Get current value of the nebula circle color.
	const Vec3f getCirclesColor(void) const;

	//! Set the default color used to draw the region symbols (default dashed shape).
	//! @param c The color of the region symbols
	//! @code
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(0.6, 0.8, 0.0);
	//! NebulaMgr.setRegionsColor(c.toVec3f());
	//! @endcode
	void setRegionsColor(const Vec3f& c);
	//! Get current value of the region color.
	const Vec3f getRegionsColor(void) const;

	//! Set the color used to draw the galaxy symbols (ellipses).
	//! @param c The color of the galaxy symbols
	//! @code
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(1.0, 0.0, 0.0);
	//! NebulaMgr.setGalaxyColor(c.toVec3f());
	//! @endcode
	void setGalaxyColor(const Vec3f& c);
	//! Get current value of the galaxy symbol color.
	const Vec3f getGalaxyColor(void) const;

	//! Set the color used to draw the active galaxy symbols (ellipses).
	//! @param c The color of the active galaxy symbols
	//! @code
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(1.0, 0.0, 0.0);
	//! NebulaMgr.setActiveGalaxyColor(c.toVec3f());
	//! @endcode
	void setActiveGalaxyColor(const Vec3f& c);
	//! Get current value of the active galaxy symbol color.
	const Vec3f getActiveGalaxyColor(void) const;

	//! Set the color used to draw the interacting galaxy symbols (ellipses).
	//! @param c The color of the interacting galaxy symbols
	//! @code
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(1.0, 0.0, 0.0);
	//! NebulaMgr.setInteractingGalaxyColor(c.toVec3f());
	//! @endcode
	void setInteractingGalaxyColor(const Vec3f& c);
	//! Get current value of the interacting galaxy symbol color.
	const Vec3f getInteractingGalaxyColor(void) const;

	//! Set the color used to draw the radio galaxy symbols (ellipses).
	//! @param c The color of the radio galaxy symbols
	//! @code
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(1.0, 0.0, 0.0);
	//! NebulaMgr.setRadioGalaxyColor(c.toVec3f());
	//! @endcode
	void setRadioGalaxyColor(const Vec3f& c);
	//! Get current value of the radio galaxy symbol color.
	const Vec3f getRadioGalaxyColor(void) const;

	//! Set the color used to draw the quasars symbols (ellipses).
	//! @param c The color of the quasars symbols
	//! @code
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(1.0, 0.0, 0.0);
	//! NebulaMgr.setQuasarColor(c.toVec3f());
	//! @endcode
	void setQuasarColor(const Vec3f& c);
	//! Get current value of the quasar symbol color.
	const Vec3f getQuasarColor(void) const;

	//! Set the color used to draw the bright nebula symbols (emission nebula boxes, planetary nebulae circles).
	//! @param c The color of the nebula symbols
	//! @code
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(0.0, 1.0, 0.0);
	//! NebulaMgr.setNebulaColor(c.toVec3f());
	//! @endcode
	void setNebulaColor(const Vec3f& c);
	//! Get current value of the nebula circle color.
	const Vec3f getNebulaColor(void) const;

	//! Set the color used to draw the planetary nebulae symbols.
	//! @param c The color of the planetary nebulae symbols
	//! @code
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(0.0, 1.0, 0.0);
	//! NebulaMgr.setPlanetaryNebulaColor(c.toVec3f());
	//! @endcode
	void setPlanetaryNebulaColor(const Vec3f& c);
	//! Get current value of the planetary nebula circle color.
	const Vec3f getPlanetaryNebulaColor(void) const;

	//! Set the color used to draw the reflection nebulae symbols.
	//! @param c The color of the reflection nebulae symbols
	//! @code
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(0.0, 1.0, 0.0);
	//! NebulaMgr.setReflectionNebulaColor(c.toVec3f());
	//! @endcode
	void setReflectionNebulaColor(const Vec3f& c);
	//! Get current value of the reflection nebula circle color.
	const Vec3f getReflectionNebulaColor(void) const;

	//! Set the color used to draw the bipolar nebulae symbols.
	//! @param c The color of the bipolar nebulae symbols
	//! @code
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(0.0, 1.0, 0.0);
	//! NebulaMgr.setBipolarNebulaColor(c.toVec3f());
	//! @endcode
	void setBipolarNebulaColor(const Vec3f& c);
	//! Get current value of the bipolar nebula circle color.
	const Vec3f getBipolarNebulaColor(void) const;

	//! Set the color used to draw the emission nebulae symbols.
	//! @param c The color of the emission nebulae symbols
	//! @code
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(0.0, 1.0, 0.0);
	//! NebulaMgr.setEmissionNebulaColor(c.toVec3f());
	//! @endcode
	void setEmissionNebulaColor(const Vec3f& c);
	//! Get current value of the emission nebula circle color.
	const Vec3f getEmissionNebulaColor(void) const;

	//! Set the color used to draw the ionized hydrogen region symbols.
	//! @param c The color of the ionized hydrogen region symbols
	//! @code
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(0.0, 1.0, 0.0);
	//! NebulaMgr.setHydrogenRegionColor(c.toVec3f());
	//! @endcode
	void setHydrogenRegionColor(const Vec3f& c);
	//! Get current value of the hydrogen region symbol color.
	const Vec3f getHydrogenRegionColor(void) const;

	//! Set the color used to draw the supernova remnant symbols.
	//! @param c The color of the supernova remnant symbols
	//! @code
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(0.0, 1.0, 0.0);
	//! NebulaMgr.setSupernovaRemnantColor(c.toVec3f());
	//! @endcode
	void setSupernovaRemnantColor(const Vec3f& c);
	//! Get current value of the supernova remnant symbol color.
	const Vec3f getSupernovaRemnantColor(void) const;

	//! Set the color used to draw the supernova candidate symbols.
	//! @param c The color of the supernova candidate symbols
	//! @code
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(0.0, 1.0, 0.0);
	//! NebulaMgr.setSupernovaCandidateColor(c.toVec3f());
	//! @endcode
	void setSupernovaCandidateColor(const Vec3f& c);
	//! Get current value of the supernova candidate symbol color.
	const Vec3f getSupernovaCandidateColor(void) const;


	//! Set the color used to draw the supernova remnant candidate symbols.
	//! @param c The color of the supernova remnant candidate symbols
	//! @code
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(0.0, 1.0, 0.0);
	//! NebulaMgr.setSupernovaRemnantCandidateColor(c.toVec3f());
	//! @endcode
	void setSupernovaRemnantCandidateColor(const Vec3f& c);
	//! Get current value of the supernova remnant candidate symbol color.
	const Vec3f getSupernovaRemnantCandidateColor(void) const;

	//! Set the color used to draw the interstellar matter symbols.
	//! @param c The color of the interstellar matter symbols
	//! @code
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(0.0, 1.0, 0.0);
	//! NebulaMgr.setInterstellarMatterColor(c.toVec3f());
	//! @endcode
	void setInterstellarMatterColor(const Vec3f& c);
	//! Get current value of the interstellar matter symbol color.
	const Vec3f getInterstellarMatterColor(void) const;

	//! Set the color used to draw the dark nebula symbols (gray boxes).
	//! @param c The color of the dark nebula symbols
	//! @code
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(0.2, 0.2, 0.2);
	//! NebulaMgr.setDarkNebulaColor(c.toVec3f());
	//! @endcode
	void setDarkNebulaColor(const Vec3f& c);
	//! Get current value of the dark nebula color.
	const Vec3f getDarkNebulaColor(void) const;

	//! Set the color used to draw the star cluster symbols (Open/Globular).
	//! @param c The color of the cluster symbols
	//! @code
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(1.0, 1.0, 0.0);
	//! NebulaMgr.setClusterColor(c.toVec3f());
	//! @endcode
	void setClusterColor(const Vec3f& c);
	//! Get current value of the star cluster symbol color.
	const Vec3f getClusterColor(void) const;

	//! Set the color used to draw the open star cluster symbols.
	//! @param c The color of the open star cluster symbols
	//! @code
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(1.0, 1.0, 0.0);
	//! NebulaMgr.setOpenClusterColor(c.toVec3f());
	//! @endcode
	void setOpenClusterColor(const Vec3f& c);
	//! Get current value of the open star cluster symbol color.
	const Vec3f getOpenClusterColor(void) const;

	//! Set the color used to draw the globular star cluster symbols.
	//! @param c The color of the globular star cluster symbols
	//! @code
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(1.0, 1.0, 0.0);
	//! NebulaMgr.setGlobularClusterColor(c.toVec3f());
	//! @endcode
	void setGlobularClusterColor(const Vec3f& c);
	//! Get current value of the globular star cluster symbol color.
	const Vec3f getGlobularClusterColor(void) const;

	//! Set the color used to draw the stellar associations symbols.
	//! @param c The color of the stellar associations symbols
	//! @code
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(1.0, 1.0, 0.0);
	//! NebulaMgr.setStellarAssociationColor(c.toVec3f());
	//! @endcode
	void setStellarAssociationColor(const Vec3f& c);
	//! Get current value of the stellar association symbol color.
	const Vec3f getStellarAssociationColor(void) const;

	//! Set the color used to draw the star clouds symbols.
	//! @param c The color of the star clouds symbols
	//! @code
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(1.0, 1.0, 0.0);
	//! NebulaMgr.setStarCloudColor(c.toVec3f());
	//! @endcode
	void setStarCloudColor(const Vec3f& c);
	//! Get current value of the star cloud symbol color.
	const Vec3f getStarCloudColor(void) const;

	//! Set the color used to draw the emission objects symbols.
	//! @param c The color of the emission objects symbols
	//! @code
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(1.0, 0.0, 0.0);
	//! NebulaMgr.setEmissionObjectColor(c.toVec3f());
	//! @endcode
	void setEmissionObjectColor(const Vec3f& c);
	//! Get current value of the emission object symbol color.
	const Vec3f getEmissionObjectColor(void) const;

	//! Set the color used to draw the BL Lac objects symbols.
	//! @param c The color of the BL Lac objects symbols
	//! @code
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(1.0, 1.0, 0.0);
	//! NebulaMgr.setBlLacObjectColor(c.toVec3f());
	//! @endcode
	void setBlLacObjectColor(const Vec3f& c);
	//! Get current value of the BL Lac object symbol color.
	const Vec3f getBlLacObjectColor(void) const;

	//! Set the color used to draw the blazars symbols.
	//! @param c The color of the blazars symbols
	//! @code
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(1.0, 1.0, 0.0);
	//! NebulaMgr.setBlazarColor(c.toVec3f());
	//! @endcode
	void setBlazarColor(const Vec3f& c);
	//! Get current value of the blazar symbol color.
	const Vec3f getBlazarColor(void) const;

	//! Set the color used to draw the molecular clouds symbols.
	//! @param c The color of the molecular clouds symbols
	//! @code
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(1.0, 1.0, 0.0);
	//! NebulaMgr.setMolecularCloudColor(c.toVec3f());
	//! @endcode
	void setMolecularCloudColor(const Vec3f& c);
	//! Get current value of the molecular cloud symbol color.
	const Vec3f getMolecularCloudColor(void) const;

	//! Set the color used to draw the young stellar objects symbols.
	//! @param c The color of the young stellar objects symbols
	//! @code
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(1.0, 1.0, 0.0);
	//! NebulaMgr.setYoungStellarObjectColor(c.toVec3f());
	//! @endcode
	void setYoungStellarObjectColor(const Vec3f& c);
	//! Get current value of the young stellar object symbol color.
	const Vec3f getYoungStellarObjectColor(void) const;

	//! Set the color used to draw the possible quasars symbols.
	//! @param c The color of the possible quasars symbols
	//! @code
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(1.0, 1.0, 0.0);
	//! NebulaMgr.setPossibleQuasarColor(c.toVec3f());
	//! @endcode
	void setPossibleQuasarColor(const Vec3f& c);
	//! Get current value of the possible quasar symbol color.
	const Vec3f getPossibleQuasarColor(void) const;

	//! Set the color used to draw the possible planetary nebulae symbols.
	//! @param c The color of the possible planetary nebulae symbols
	//! @code
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(1.0, 1.0, 0.0);
	//! NebulaMgr.setPossiblePlanetaryNebulaColor(c.toVec3f());
	//! @endcode
	void setPossiblePlanetaryNebulaColor(const Vec3f& c);
	//! Get current value of the possible planetary nebula symbol color.
	const Vec3f getPossiblePlanetaryNebulaColor(void) const;

	//! Set the color used to draw the protoplanetary nebulae symbols.
	//! @param c The color of the protoplanetary nebulae symbols
	//! @code
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(1.0, 1.0, 0.0);
	//! NebulaMgr.setProtoplanetaryNebulaColor(c.toVec3f());
	//! @endcode
	void setProtoplanetaryNebulaColor(const Vec3f& c);
	//! Get current value of the protoplanetary nebula symbol color.
	const Vec3f getProtoplanetaryNebulaColor(void) const;

	//! Set the color used to draw the stars symbols.
	//! @param c The color of the stars symbols
	//! @code
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(1.0, 1.0, 0.0);
	//! NebulaMgr.setStarColor(c.toVec3f());
	//! @endcode
	void setStarColor(const Vec3f& c);
	//! Get current value of the star symbol color.
	const Vec3f getStarColor(void) const;

	//! Set the color used to draw the symbiotic stars symbols.
	//! @param c The color of the symbiotic stars symbols
	//! @code
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(1.0, 1.0, 0.0);
	//! NebulaMgr.setSymbioticStarColor(c.toVec3f());
	//! @endcode
	void setSymbioticStarColor(const Vec3f& c);
	//! Get current value of the symbiotic star symbol color.
	const Vec3f getSymbioticStarColor(void) const;

	//! Set the color used to draw the emission-line stars symbols.
	//! @param c The color of the emission-line stars symbols
	//! @code
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(1.0, 1.0, 0.0);
	//! NebulaMgr.setEmissionLineStarColor(c.toVec3f());
	//! @endcode
	void setEmissionLineStarColor(const Vec3f& c);
	//! Get current value of the emission-line star symbol color.
	const Vec3f getEmissionLineStarColor(void) const;

	//! Set the color used to draw the cluster of galaxies symbols.
	//! @param c The color of the cluster of galaxies symbols
	//! @code
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(1.0, 1.0, 0.0);
	//! NebulaMgr.setGalaxyClusterColor(c.toVec3f());
	//! @endcode
	void setGalaxyClusterColor(const Vec3f& c);
	//! Get current value of the cluster of galaxies symbol color.
	const Vec3f getGalaxyClusterColor(void) const;

	//! Set how long it takes for nebula hints to fade in and out when turned on and off.
	//! @param duration given in seconds
	void setHintsFadeDuration(float duration) {hintsFader.setDuration(static_cast<int>(duration * 1000.f));}

	//! Set flag for displaying Nebulae Hints.
	void setFlagHints(bool b) { if (hintsFader!=b) { hintsFader=b; emit flagHintsDisplayedChanged(b);}}
	//! Get flag for displaying Nebulae Hints.
	bool getFlagHints(void) const {return hintsFader;}

	//! Set whether hints (symbols) should be scaled according to nebula size.
	void setHintsProportional(const bool proportional);
	//! Get whether hints (symbols) are scaled according to nebula size.
	bool getHintsProportional(void) const;

	//! Set flag for usage outlines for big DSO instead their hints.
	void setFlagOutlines(const bool flag);
	//! Get flag for usage outlines for big DSO instead their hints.
	bool getFlagOutlines(void) const;

	//! Set flag for show an additional names for DSO
	void setFlagAdditionalNames(const bool flag);
	//! Get flag for show an additional names for DSO
	bool getFlagAdditionalNames(void) const;

	//! Set flag for usage designations of DSO for their labels instead common names.
	void setDesignationUsage(const bool flag);
	//! Get flag for usage designations of DSO for their labels instead common names.
	bool getDesignationUsage(void) const;

	//! Set whether hints (symbols) should be visible according to surface brightness value.
	void setFlagSurfaceBrightnessUsage(const bool usage);
	//! Get whether hints (symbols) are visible according to surface brightness value.
	bool getFlagSurfaceBrightnessUsage(void) const;

	//! Set flag for usage of measure unit mag/arcsec^2 to surface brightness value.
	void setFlagSurfaceBrightnessArcsecUsage(const bool usage);
	//! Get flag for usage of measure unit mag/arcsec^2 to surface brightness value.
	bool getFlagSurfaceBrightnessArcsecUsage(void) const;

	//! Set flag for usage of short notation for measure unit to surface brightness value.
	void setFlagSurfaceBrightnessShortNotationUsage(const bool usage);
	//! Get flag for usage of short notation for measure unit to surface brightness value.
	bool getFlagSurfaceBrightnessShortNotationUsage(void) const;

	//! Set flag for usage of size limits.
	void setFlagSizeLimitsUsage(const bool usage);
	//! Get flag for usage of size limits.
	bool getFlagSizeLimitsUsage(void) const;

	//! Set flag used to turn on and off Nebula rendering.
	void setFlagShow(bool b) { flagShow = b; }
	//! Get value of flag used to turn on and off Nebula rendering.
	bool getFlagShow(void) const { return flagShow; }

	//! Set flag used to turn on and off DSO type filtering.
	void setFlagUseTypeFilters(const bool b);
	//! Get value of flag used to turn on and off DSO type filtering.
	bool getFlagUseTypeFilters(void) const;

	//! Set flag used to turn on and off Named DSO Only.
	void setFlagShowOnlyNamedDSO(const bool b);
	//! Get value of flag used to turn on and off Named DSO Only.
	bool getFlagShowOnlyNamedDSO(void) const;

	//! Set the limit for min. angular size of displayed DSO.
	//! @param s the angular size between 1 and 600 arcminutes
	void setMinSizeLimit(double s);
	//! Get the limit for min. angular size of displayed DSO.
	//! @return the angular size between 1 and 600 arcminutes
	double getMinSizeLimit(void) const;

	//! Set the limit for max. angular size of displayed DSO.
	//! @param s the angular size between 1 and 600 arcminutes
	void setMaxSizeLimit(double s);
	//! Get the limit for max. angular size of displayed DSO.
	//! @return the angular size between 1 and 600 arcminutes
	double getMaxSizeLimit(void) const;

	//! Set the color used to draw nebula labels.
	//! @param c The color of the nebula labels
	//! @code
	//! // example of usage in scripts
	//! NebulaMgr.setLabelsColor(Vec3f(1.0,0.0,0.0));
	//! @endcode
	void setLabelsColor(const Vec3f& c);
	//! Get current value of the nebula label color.
	const Vec3f getLabelsColor(void) const;

	//! Set the amount of nebulae labels. The real amount is also proportional with FOV.
	//! The limit is set in function of the nebulae magnitude
	//! @param a the amount between 0 and 10. 0 is no labels, 10 is maximum of labels
	void setLabelsAmount(double a);
	//! Get the amount of nebulae labels. The real amount is also proportional with FOV.
	//! @return the amount between 0 and 10. 0 is no labels, 10 is maximum of labels
	double getLabelsAmount(void) const;

	//! Set the amount of nebulae hints. The real amount is also proportional with FOV.
	//! The limit is set in function of the nebulae magnitude
	//! @param f the amount between 0 and 10. 0 is no hints, 10 is maximum of hints
	void setHintsAmount(double f);
	//! Get the amount of nebulae labels. The real amount is also proportional with FOV.
	//! @return the amount between 0 and 10. 0 is no hints, 10 is maximum of hints
	double getHintsAmount(void) const;

	//! Set the brightness used to paint nebulae labels.
	//! @param a the amount between 0 and 1. 0 is black, 1 is maximum brightness
	void setLabelsBrightness(double b);
	//! Get the brightness used to paint nebulae labels.
	//! @return the amount between 0 and 1. 0 is dark (no labels), 1 is maximum brightness of labels
	double getLabelsBrightness(void) const;

	//! Set the brightness of nebulae hints.
	//! @param f the amount between 0 and 10. 0 is no hints, 10 is maximum of hints
	void setHintsBrightness(double b);
	//! Get the brightness of nebulae labels.
	//! @return the amount between 0 and 1. 0 is dark (no hints), 1 is maximum brightness of hints
	double getHintsBrightness(void) const;


signals:
	//! Emitted when hints are toggled.
	void flagHintsDisplayedChanged(bool b);
	//! Emitted when filter types are changed.
	void flagUseTypeFiltersChanged(bool b);
	//! Emitted when the catalog filter is changed
	void catalogFiltersChanged(int flags); // emits an int cast of Nebula::CatalogGroup
	//! Emitted when the type filter is changed
	void typeFiltersChanged(int flags); // emits an int cast of Nebula::TypeGroup
	void hintsProportionalChanged(bool b);
	void flagOutlinesDisplayedChanged(bool b);
	void flagAdditionalNamesDisplayedChanged(bool b);
	void designationUsageChanged(bool b);
	void flagShowOnlyNamedDSOChanged(bool b);
	void flagSurfaceBrightnessUsageChanged(bool b);
	void flagSurfaceBrightnessArcsecUsageChanged(bool b);
	void flagSurfaceBrightnessShortNotationUsageChanged(bool b);
	void flagSizeLimitsUsageChanged(bool b);
	void minSizeLimitChanged(double s);
	void maxSizeLimitChanged(double s);
	void labelsAmountChanged(double a);
	void hintsAmountChanged(double f);
	void labelsBrightnessChanged(double b);
	void hintsBrightnessChanged(double b);

	void labelsColorChanged(const Vec3f & color) const;
	void circlesColorChanged(const Vec3f & color) const;
	void regionsColorChanged(const Vec3f & color) const;
	void galaxiesColorChanged(const Vec3f & color) const;
	void activeGalaxiesColorChanged(const Vec3f & color) const;
	void radioGalaxiesColorChanged(const Vec3f & color) const;
	void interactingGalaxiesColorChanged(const Vec3f & color) const;
	void quasarsColorChanged(const Vec3f & color) const;
	void possibleQuasarsColorChanged(const Vec3f & color) const;
	void clustersColorChanged(const Vec3f & color) const;
	void openClustersColorChanged(const Vec3f & color) const;
	void globularClustersColorChanged(const Vec3f & color) const;
	void stellarAssociationsColorChanged(const Vec3f & color) const;
	void starCloudsColorChanged(const Vec3f & color) const;
	void starsColorChanged(const Vec3f & color) const;
	void symbioticStarsColorChanged(const Vec3f & color) const;
	void emissionLineStarsColorChanged(const Vec3f & color) const;
	void nebulaeColorChanged(const Vec3f & color) const;
	void planetaryNebulaeColorChanged(const Vec3f & color) const;
	void darkNebulaeColorChanged(const Vec3f & color) const;
	void reflectionNebulaeColorChanged(const Vec3f & color) const;
	void bipolarNebulaeColorChanged(const Vec3f & color) const;
	void emissionNebulaeColorChanged(const Vec3f & color) const;
	void possiblePlanetaryNebulaeColorChanged(const Vec3f & color) const;
	void protoplanetaryNebulaeColorChanged(const Vec3f & color) const;
	void hydrogenRegionsColorChanged(const Vec3f & color) const;
	void interstellarMatterColorChanged(const Vec3f & color) const;
	void emissionObjectsColorChanged(const Vec3f & color) const;
	void molecularCloudsColorChanged(const Vec3f & color) const;
	void blLacObjectsColorChanged(const Vec3f & color) const;
	void blazarsColorChanged(const Vec3f & color) const;
	void youngStellarObjectsColorChanged(const Vec3f & color) const;
	void supernovaRemnantsColorChanged(const Vec3f & color) const;
	void supernovaCandidatesColorChanged(const Vec3f & color) const;
	void supernovaRemnantCandidatesColorChanged(const Vec3f & color) const;
	void galaxyClustersColorChanged(const Vec3f & color) const;

private slots:
	//! Update i18 names from English names according to passed translator.
	//! The translation is done using gettext with translated strings defined
	//! in translations.h
	void updateI18n();
	
	//! Called when the sky culture is updated.
	//! Loads native names of deep-sky objects for a given sky culture.
	void updateSkyCulture(const StelSkyCulture& skyCulture);

	int loadCultureSpecificNames(const QJsonObject& data);
	void loadCultureSpecificNameForNamedObject(const QJsonArray& data, const QString& commonName);

	//! Connect from StelApp to reflect font size change.
	void setFontSizeFromApp(int size){nebulaFont.setPixelSize(size);}

private:
	//! Search for a nebula object by name, e.g. M83, NGC 1123, IC 1234.
	NebulaP searchForCommonName(const QString& name);

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
	void drawPointer(const StelCore* core, StelPainter& sPainter);

	NebulaP searchDSO(unsigned int DSO) const;
	NebulaP searchM(unsigned int M) const;
	NebulaP searchNGC(unsigned int NGC) const;
	NebulaP searchIC(unsigned int IC) const;
	NebulaP searchC(unsigned int C) const;
	NebulaP searchB(unsigned int B) const;
	NebulaP searchSh2(unsigned int Sh2) const;
	NebulaP searchVdB(unsigned int VdB) const;
	NebulaP searchRCW(unsigned int RCW) const;
	NebulaP searchLDN(unsigned int LDN) const;
	NebulaP searchLBN(unsigned int LBN) const;
	NebulaP searchCr(unsigned int Cr) const;
	NebulaP searchMel(unsigned int Mel) const;
	NebulaP searchPGC(unsigned int PGC) const;
	NebulaP searchUGC(unsigned int UGC) const;
	NebulaP searchCed(QString Ced) const;
	NebulaP searchArp(unsigned int Arp) const;
	NebulaP searchVV(unsigned int VV) const;
	NebulaP searchPK(QString PK) const;
	NebulaP searchPNG(QString PNG) const;
	NebulaP searchSNRG(QString SNRG) const;
	NebulaP searchACO(QString ACO) const;
	NebulaP searchHCG(QString HCG) const;	
	NebulaP searchESO(QString ESO) const;
	NebulaP searchVdBH(QString VdBH) const;
	NebulaP searchDWB(unsigned int DWB) const;
	NebulaP searchTr(unsigned int Tr) const;
	NebulaP searchSt(unsigned int St) const;
	NebulaP searchRu(unsigned int Ru) const;
	NebulaP searchVdBHa(unsigned int VdBHa) const;
	//! Return the matching nebula if exists or Q_NULLPTR.
	//! @param name The case in-sensitive designation of deep-sky object
	NebulaP searchByDesignation(const QString& designation) const;

	// Load catalog of DSO
	bool loadDSOCatalog(const QString& filename);
	void convertDSOCatalog(const QString& in, const QString& out, bool decimal);
	// Load proper names for DSO
	bool loadDSONames(const QString& filename);
	// Load outlines for DSO
	bool loadDSOOutlines(const QString& filename);
	// Load discovery data for DSO
	// TODO: Move these data into main DSO catalog to fast reading (v4)
	bool loadDSODiscoveryData(const QString& filename);

	//! User-chosen suppressed references.
	//! The collection of DSO names in default/names.dat does not suit all.
	//! We allow to block loading references.
	//! The unwanted references go to config.ini: astro/nebula_exclude_references = default:unref1,unref2,...;mySet:unref7,unref9,...
	//! In order to load new versions of the ever-growing list of names (with growing list of references),
	//! we store which entries should be blocked.
	//! These names only block the global list, not skyculture-added names.
	QStringList unwantedReferences;

	QVector<NebulaP> dsoArray;		// The DSO list
	QHash<unsigned int, NebulaP> dsoIndex;
	// A struct that holds a NebulaP and user-accepted references from the loaded nebulaSet (usually nebulae/default/names.dat)
	struct NebulaWithReferences {
		NebulaP nebula;
		QStringList references;
	};
	//! A Hash that holds a QStringList of all "accepted" names from nebulae/default/names.dat
	//! @note "accepted" means that users can exclude DSO listed only in selected (unwanted) references.
	QHash<NebulaP,QStringList> commonNameMap;
	//! A Hash that holds NebulaP for each "accepted" uppercased name from nebulae/default/names.dat
	//! @note "accepted" means that users can exclude DSO listed only in selected (unwanted) references.
	QHash<QString/*name*/,NebulaWithReferences> commonNameIndex;

	LinearFader hintsFader;
	LinearFader flagShow;

	//! The internal grid for fast positional lookup
	StelSphericalIndex nebGrid;

	//! The amount of hints (between 0 and 10)
	double hintsAmount;
	//! The amount of labels (between 0 and 10)
	double labelsAmount;

	//! The max brightness setting of hints (between 0 and 1)
	double hintsBrightness;
	//! The max brightness setting of labels (between 0 and 1)
	double labelsBrightness;

	//! The selection pointer texture
	StelTextureSP texPointer;
	
	QFont nebulaFont;      // Font used for names printing

	// For DSO converter
	bool flagConverter;
	bool flagDecimalCoordinates;

	static const QString StellariumDSOCatalogVersion;
};

#endif // NEBULAMGR_HPP
