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

#ifndef STARMGR_HPP
#define STARMGR_HPP

#include <QFont>
#include <QVariantMap>
#include <QVector>
#include "StelFader.hpp"
#include "StelObjectModule.hpp"
#include "StelTextureTypes.hpp"
#include "StelObject.hpp"

class StelObject;
class StelToneReproducer;
class StelSkyCulture;
class StelProjector;
class StelPainter;
class QSettings;

class ZoneArray;
struct HipIndexStruct;

static const int RCMAG_TABLE_SIZE = 512;

typedef struct
{
	QString designation;	//!< GCVS designation
	QString vtype;		//!< Type of variability
	float maxmag;		//!< Magnitude at maximum brightness
	int mflag;		//!< Magnitude flag code
	float min1mag;		//!< First minimum magnitude or amplitude
	float min2mag;		//!< Second minimum magnitude or amplitude
	QString photosys;	//!< The photometric system for magnitudes
	double epoch;		//!< Epoch for maximum light (Julian days)
	double period;		//!< Period of the variable star (days)
	int Mm;			//!< Rising time or duration of eclipse (%)
	QString stype;		//!< Spectral type
} varstar;

typedef struct
{
	QString designation;	//!< WDS designation
	int observation;	//!< Date of last satisfactory observation, yr
	float positionAngle;	//!< Position Angle at date of last satisfactory observation, deg
	float separation;	//!< Separation at date of last satisfactory observation, arcsec
} wds;

typedef struct
{
	int sao;                //!< SAO Smithsonian astrophysical Observatory
	int hd;                 //!< HD Henry Draper catalog
	int hr;                 //!< HR Harvard Revised Photometry Catalogue, now Yale Bright Star Catalogue
} crossid;

typedef QPair<StelObjectP, float> StelACStarData;
typedef uint64_t StarId;

typedef struct
{
	StarId hip;
	bool primary;
	double binary_period;
	float eccentricity;
	float inclination;
	float big_omega;
	float small_omega;
	double periastron_epoch;
	double semi_major;
	double bary_distance;
	double data_epoch;
	double bary_ra;
	double bary_dec;
	double bary_rv;
	double bary_pmra;
	double bary_pmdec;
	float primary_mass;
	float secondary_mass;
} binaryorbitstar;


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
	Q_PROPERTY(bool flagStarsDisplayed
		   READ getFlagStars
		   WRITE setFlagStars
		   NOTIFY starsDisplayedChanged)
	Q_PROPERTY(bool flagLabelsDisplayed
		   READ getFlagLabels
		   WRITE setFlagLabels
		   NOTIFY starLabelsDisplayedChanged)
	Q_PROPERTY(double labelsAmount
		   READ getLabelsAmount
		   WRITE setLabelsAmount
		   NOTIFY labelsAmountChanged)
	Q_PROPERTY(bool flagAdditionalNamesDisplayed
		   READ getFlagAdditionalNames
		   WRITE setFlagAdditionalNames
		   NOTIFY flagAdditionalNamesDisplayedChanged
		   )
	Q_PROPERTY(bool flagDesignationLabels
		   READ getDesignationUsage
		   WRITE setDesignationUsage
		   NOTIFY designationUsageChanged
		   )
	Q_PROPERTY(bool flagDblStarsDesignation
		   READ getFlagDblStarsDesignation
		   WRITE setFlagDblStarsDesignation
		   NOTIFY flagDblStarsDesignationChanged
		   )
	Q_PROPERTY(bool flagVarStarsDesignation
		   READ getFlagVarStarsDesignation
		   WRITE setFlagVarStarsDesignation
		   NOTIFY flagVarStarsDesignationChanged
		   )
	Q_PROPERTY(bool flagHIPDesignation
		   READ getFlagHIPDesignation
		   WRITE setFlagHIPDesignation
		   NOTIFY flagHIPDesignationChanged
		   )

public:
	StarMgr(void);
	~StarMgr(void) override;

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	//! Initialize the StarMgr.
	//! - Loads the star catalogue data into memory
	//! - Sets up the star color table
	//! - Loads the star texture
	//! - Loads the star font (for labels on named stars)
	//! - Loads the texture of the star selection indicator
	//! - Sets various display flags from the ini parser object
	void init() override;

	//! Draw the stars and the star selection indicator if necessary.
	void draw(StelCore* core) override;

	//! Update any time-dependent features.
	//! Includes fading in and out stars and labels when they are turned on and off.
	void update(double deltaTime) override {labelsFader.update(static_cast<int>(deltaTime*1000)); starsFader.update(static_cast<int>(deltaTime*1000));}

	//! Used to determine the order in which the various StelModules are drawn.
	double getCallOrder(StelModuleActionName actionName) const override;

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in StelObjectModule class
	//! Return a list containing the stars located inside the limFov circle around position v
	QList<StelObjectP > searchAround(const Vec3d& v, double limitFov, const StelCore* core) const override;

	//! Return the matching Stars object's pointer if exists or Q_NULLPTR
	//! @param nameI18n The case in-sensitive localized star common name or HIP/HP, SAO, HD, HR, GCVS or WDS number
	//! catalog name (format can be HP1234 or HP 1234 or HIP 1234) or sci name
	StelObjectP searchByNameI18n(const QString& nameI18n) const override;

	//! Return the matching star if exists or Q_NULLPTR
	//! @param name The case in-sensitive english star name
	StelObjectP searchByName(const QString& name) const override;

	//! Same as searchByName(id);
	StelObjectP searchByID(const QString &id) const override;

	//! Find and return the list of at most maxNbItem objects auto-completing the passed object English name.
	//! @param objPrefix the case insensitive first letters of the searched object
	//! @param maxNbItem the maximum number of returned object names
	//! @param useStartOfWords the autofill mode for returned objects names
	//! @return a list of matching object name by order of relevance, or an empty list if nothing match
	QStringList listMatchingObjects(const QString& objPrefix, int maxNbItem=5, bool useStartOfWords=false) const override;
	//! List all currently loaded names.
	//! @param inEnglish list EnglishNames (true) or translated (false)
	//! @return a list of matching object name by order of relevance, or an empty list if nothing matches
	//! @note Listing stars with the common names only, not skyculture-related.
	QStringList listAllObjects(bool inEnglish) const override;
	//! @param objType a string with int number 0...8.
	//! 0..Interesting double stars
	//! 1..Interesting variable stars
	//! 2..Bright double stars
	//! 3..Bright variable stars
	//! 4..high proper motion stars
	//! 5..Algol-type eclipsing systems
	//! 6..Classical delta Cepheid stars
	//! 7..Bright carbon stars
	//! 8..Bright Barium stars
	//! @param inEnglish: return English, not translated star names
	//! @return a QStringList with all known star names
	QStringList listAllObjectsByType(const QString& objType, bool inEnglish) const override;
	QString getName() const override { return "Stars"; }
	//! @return "Star"
	QString getStelObjectType() const override;

public slots:
	///////////////////////////////////////////////////////////////////////////
	// Methods callable from script and GUI
	//! Set display flag for Stars.
	void setFlagStars(bool b);
	//! Get display flag for Stars
	bool getFlagStars(void) const {return starsFader==true;}

	//! Set display flag for Star names (labels).
	void setFlagLabels(bool b);
	//! Get display flag for Star names (labels).
	bool getFlagLabels(void) const {return labelsFader==true;}

	//! Set the amount of star labels. The real amount is also proportional with FOV.
	//! The limit is set in function of the stars magnitude
	//! @param a the amount between 0 and 10. 0 is no labels, 10 is maximum of labels
	void setLabelsAmount(double a);
	//! Get the amount of star labels. The real amount is also proportional with FOV.
	//! @return the amount between 0 and 10. 0 is no labels, 10 is maximum of labels
	double getLabelsAmount(void) const {return labelsAmount;}

	//! Define font size to use for star names display.
	void setFontSize(int newFontSize);

	//! Show scientific or catalog names on stars without common names.
	static void setFlagSciNames(bool f) {flagSciNames = f;}
	static bool getFlagSciNames(void) {return flagSciNames;}

	//! Set flag for usage designations of stars for their labels instead common names.
	void setDesignationUsage(const bool flag);
	//! Get flag for usage designations of stars for their labels instead common names.
	static bool getDesignationUsage(void) {return flagDesignations; }

	//! Set flag for usage traditional designations of double stars.
	void setFlagDblStarsDesignation(const bool flag);
	//! Get flag for usage traditional designations of double stars.
	static bool getFlagDblStarsDesignation(void) {return flagDblStarsDesignation; }

	//! Set flag for usage designations of variable stars.
	void setFlagVarStarsDesignation(const bool flag);
	//! Get flag for usage designations of variable stars.
	static bool getFlagVarStarsDesignation(void) {return flagVarStarsDesignation; }

	//! Set flag for usage Hipparcos catalog designations of stars.
	void setFlagHIPDesignation(const bool flag);
	//! Get flag for usage Hipparcos catalog designations of stars.
	static bool getFlagHIPDesignation(void) {return flagHIPDesignation; }

	//! Show additional star names.
	void setFlagAdditionalNames(bool flag);
	static bool getFlagAdditionalNames(void) { return flagAdditionalStarNames; }

public:
	///////////////////////////////////////////////////////////////////////////
	// Other methods
	//! Search by Hipparcos catalogue number.
	//! @param hip the Hipparcos catalogue number of the star which is required.
	//! @return the requested StelObjectP or an empty objecy if the requested
	//! one was not found.
	StelObjectP searchHP(int hip) const;

	//! Search by Gaia source id.
	//! @param source_id the Gaia source id of the star which is required.
	//! @return the requested StelObjectP or an empty objecy if the requested
	//! one was not found.
	StelObjectP searchGaia(StarId source_id) const;

	//! Get the (translated) common name for a star with a specified
	//! Hipparcos or Gaia catalogue number.
	//! @param hip The Hipparcos/Gaia number of star
	//! @return translated common name of star
	//! @todo Rename to getCommonNameI18n
	static QString getCommonName(StarId hip);

	//! Get the (translated) scientific name for a star with a specified
	//! Hipparcos or Gaia catalogue number.
	//! @param hip The Hipparcos/Gaia number of star
	//! @return translated scientific name of star
	static QString getSciName(StarId hip);

	//! Get the (translated) scientific extra name for a star with a specified
	//! Hipparcos or Gaia catalogue number.
	//! @param hip The Hipparcos/Gaia number of star
	//! @return translated scientific name of star
	static QString getSciExtraName(StarId hip);

	//! Get the (translated) scientific name for a variable star with a specified
	//! Hipparcos or Gaia catalogue number.
	//! @param hip The Hipparcos/Gaia number of star
	//! @return translated scientific name of variable star
	static QString getGcvsName(StarId hip);

	//! Get the (translated) scientific name for a double star with a specified
	//! Hipparcos or Gaia catalogue number.
	//! @param hip The Hipparcos/Gaia number of star
	//! @return translated scientific name of double star
	static QString getWdsName(StarId hip);

	//! Get the (English) common name for a star with a specified
	//! Hipparcos or Gaia catalogue number.
	//! @param hip The Hipparcos/Gaia number of star
	//! @return common name of star (from skyculture file star_names.fab)
	static QString getCommonEnglishName(StarId hip);

	//! Get the (translated) additional names for a star with a specified
	//! Hipparcos or Gaia catalogue number.
	//! @param hip The Hipparcos/Gaia number of star
	//! @return translated additional names of star
	static QString getAdditionalNames(StarId hip);

	//! Get the English additional names for a star with a specified
	//! Hipparcos or Gaia catalogue number.
	//! @param hip The Hipparcos/Gaia number of star
	//! @return additional names of star
	static QString getAdditionalEnglishNames(StarId hip);

	//! Get the cross-identification designations for a star with a specified
	//! Hipparcos or Gaia catalogue number.
	//! @param hip The Hipparcos/Gaia number of star
	//! @return cross-identification data
	static QString getCrossIdentificationDesignations(const QString &hip);

	//! Get the type of variability for a variable star with a specified
	//! Hipparcos or Gaia catalogue number.
	//! @param hip The Hipparcos/Gaia number of star
	//! @return type of variability
	static QString getGcvsVariabilityType(StarId hip);

	//! Get the magnitude at maximum brightness for a variable star with a specified
	//! Hipparcos or Gaia catalogue number.
	//! @param hip The Hipparcos/Gaia number of star
	//! @return the magnitude at maximum brightness for a variable star
	static float getGcvsMaxMagnitude(StarId hip);

	//! Get the magnitude flag code for a variable star with a specified
	//! Hipparcos or Gaia catalogue number.
	//! @param hip The Hipparcos/Gaia number of star
	//! @return the magnitude flag code for a variable star
	static int getGcvsMagnitudeFlag(StarId hip);

	//! Get the minimum magnitude or amplitude for a variable star with a specified
	//! Hipparcos or Gaia catalogue number.
	//! @param hip The Hipparcos/Gaia number of star
	//! @param firstMinimumFlag
	//! @return the minimum magnitude or amplitude for a variable star
	static float getGcvsMinMagnitude(StarId hip, bool firstMinimumFlag=true);

	//! Get the photometric system for a variable star with a specified
	//! Hipparcos or Gaia catalogue number.
	//! @param hip The Hipparcos/Gaia number of star
	//! @return the photometric system for a variable star
	static QString getGcvsPhotometricSystem(StarId hip);

	//! Get Epoch for maximum light for a variable star with a specified
	//! Hipparcos or Gaia catalogue number.
	//! @param hip The Hipparcos/Gaia number of star
	//! @return Epoch for maximum light for a variable star
	static double getGcvsEpoch(StarId hip);

	//! Get the period for a variable star with a specified
	//! Hipparcos or Gaia catalogue number.
	//! @param hip The Hipparcos/Gaia number of star
	//! @return the period of variable star
	static double getGcvsPeriod(StarId hip);

	//! Get the rising time or duration of eclipse for a variable star with a
	//! specified Hipparcos or Gaia catalogue number.
	//! @param hip The Hipparcos/Gaia number of star
	//! @return the rising time or duration of eclipse for variable star
	static int getGcvsMM(StarId hip);

	//! Get the spectral type of variable star with a
	//! specified Hipparcos or Gaia catalogue number.
	//! @param hip The Hipparcos/Gaia number of star
	//! @return the spectral type of variable star
	static QString getGcvsSpectralType(StarId hip);

	//! Get year of last satisfactory observation of double star with a
	//! Hipparcos or Gaia catalogue number.
	//! @param hip The Hipparcos/Gaia number of star
	//! @return year of last satisfactory observation
	static int getWdsLastObservation(StarId hip);

	//! Get position angle at date of last satisfactory observation of double star with a
	//! Hipparcos or Gaia catalogue number.
	//! @param hip The Hipparcos/Gaia number of star
	//! @return position angle in degrees
	static float getWdsLastPositionAngle(StarId hip);

	//! Get separation angle at date of last satisfactory observation of double star with a
	//! Hipparcos or Gaia catalogue number.
	//! @param hip The Hipparcos/Gaia number of star
	//! @return separation in arcseconds
	static float getWdsLastSeparation(StarId hip);

	//! Get binary orbit data for a star with a specified Hipparcos or Gaia catalogue number.
	//! @param hip The Hipparcos/Gaia number of star
	//! @return binary orbit data
	static binaryorbitstar getBinaryOrbitData(StarId hip);

	static QString convertToSpectralType(int index);
	static QString convertToComponentIds(int index);
	static QString convertToOjectTypes(int index);

	QVariantList getCatalogsDescription() const {return catalogsDescription;}

	//! Try to load the given catalog, even if it is marched as unchecked.
	//! Mark it as checked if checksum is correct.
	//! @param m the catalog data.
	//! @param load true if the catalog should be loaded, otherwise just check but don't load.
	//! @return false in case of failure.
	bool checkAndLoadCatalog(const QVariantMap& m, bool load);

	//! Get the list of all Hipparcos stars.
	const QList<StelObjectP>& getHipparcosStars() const { return hipparcosStars; }	
	const QList<QPair<StelObjectP, float>>& getHipparcosHighPMStars() const { return hipStarsHighPM; }
	const QList<QPair<StelObjectP, float>>& getHipparcosDoubleStars() const { return doubleHipStars; }
	const QList<QPair<StelObjectP, float>>& getHipparcosVariableStars() const { return variableHipStars; }
	const QList<QPair<StelObjectP, float>>& getHipparcosAlgolTypeStars() const { return algolTypeStars; }
	const QList<QPair<StelObjectP, float>>& getHipparcosClassicalCepheidsTypeStars() const { return classicalCepheidsTypeStars; }
	const QList<StelObjectP>& getHipparcosCarbonStars() const { return carbonStars; }
	const QList<StelObjectP>& getHipparcosBariumStars() const { return bariumStars; }

private slots:
	//! Translate text.
	void updateI18n();

	//! Called when the sky culture is updated.
	//! Loads common and scientific names of stars for a given sky culture.
	void updateSkyCulture(const StelSkyCulture& skyCulture);

	//! increase artificial cutoff magnitude slightly (can be linked to an action/hotkey)
	void increaseStarsMagnitudeLimit();
	//! decrease artificial cutoff magnitude slightly (can be linked to an action/hotkey)
	void reduceStarsMagnitudeLimit();

signals:
	void starLabelsDisplayedChanged(const bool displayed);
	void starsDisplayedChanged(const bool displayed);
	void designationUsageChanged(const bool flag);
	void flagDblStarsDesignationChanged(const bool flag);
	void flagVarStarsDesignationChanged(const bool flag);
	void flagHIPDesignationChanged(const bool flag);
	void flagAdditionalNamesDisplayedChanged(const bool displayed);
	void labelsAmountChanged(double a);

private:
	void setCheckFlag(const QString& catalogId, bool b);

	void copyDefaultConfigFile();

	struct CommonNames
	{
		QHash<int, QString> byHIP;
		QMap<QString, int> hipByName; // Reverse mapping of uppercased name to HIP number
	};
	//! Loads common names for stars from a file. (typical: skycultures/common_star_names.fab)
	//! Called when the SkyCulture is updated.
	//! @param the path to a file containing the common names for bright stars.
	//! @note Stellarium doesn't support sky cultures made prior version 25.1.
	CommonNames loadCommonNames(const QString& commonNameFile) const;

	//! Load culture-specific names for stars from JSON data
	void loadCultureSpecificNames(const QJsonObject& data, const QMap<QString, int>& commonNamesIndexToSearchWhileLoading);
	void loadCultureSpecificNameForStar(const QJsonArray& data, StarId HIP);
	void loadCultureSpecificNameForNamedObject(const QJsonArray& data, const QString& commonName,
	                                           const QMap<QString, int>& commonNamesIndexToSearchWhileLoading);

	//! Loads scientific names for stars from a file.
	//! Called when the SkyCulture is updated.
	//! @param the path to a file containing the scientific names for bright stars.
	//! @param flag to load the extra designations
	void loadSciNames(const QString& sciNameFile, const bool extraData);

	//! Loads GCVS from a file.
	//! @param the path to a file containing the GCVS.
	void loadGcvs(const QString& GcvsFile);

	//! Loads WDS from a file.
	//! @param the path to a file containing the WDS.
	void loadWds(const QString& WdsFile);

	//! Loads cross-identification data from a file.
	//! @param the path to a file containing the cross-identification data.
	void loadCrossIdentificationData(const QString& crossIdFile);

	//! Loads orbital parameters data for binary systems data from a file.
	//! @param the path to a file containing the orbital parameters data for binary systems.
	void loadBinaryOrbitalData(const QString& orbitalParamFile);

	//! Gets the maximum search level.
	// TODO: add a non-lame description - what is the purpose of the max search level?
	int getMaxSearchLevel() const;

	//! Load all the stars from the files.
	void loadData(QVariantMap starsConfigFile);

	//! Draw a nice animated pointer around the object.
	void drawPointer(StelPainter& sPainter, const StelCore* core);

	//! Fill hipparcosStars, hipStarsHighPM, doubleHipStars, variableHipStars, algolTypeStars,
	//! classicalCepheidsTypeStars, carbonStars, bariumStars. Called once in init().
	void populateHipparcosLists();
	//! Load scientific star names, variable names, binary data, cross indices. Called once in init().
	void populateStarsDesignations();

	//! List of all Hipparcos stars.
	QList<StelObjectP> hipparcosStars, carbonStars, bariumStars;
	// TODO: Document why this is a list of 1-element(?) QMAPs, not just a QMap itself
	QList<StelACStarData> doubleHipStars, variableHipStars, algolTypeStars, classicalCepheidsTypeStars, hipStarsHighPM;

	LinearFader labelsFader;
	LinearFader starsFader;

	bool flagStarName;	
	double labelsAmount;
	bool gravityLabel;

	int maxGeodesicGridLevel;
	int lastMaxSearchLevel;
	
	// A ZoneArray per grid level
	QVector<ZoneArray*> gridLevels;
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

	HipIndexStruct *hipIndex; // array of Hipparcos stars

	static QHash<StarId, QString> commonNamesMap;     // the original names from skyculture (star_names.fab)
	static QHash<StarId, QString> commonNamesMapI18n; // translated names
	static QMap<QString, StarId> commonNamesIndexI18n;
	static QMap<QString, StarId> commonNamesIndex;    // back-references upper-case names

	static QHash<StarId, QString> additionalNamesMap; // additional names. These will be replaced by the CulturalNames
	static QHash<StarId, QString> additionalNamesMapI18n;
	static QMap<QString, StarId> additionalNamesIndex;
	static QMap<QString, StarId> additionalNamesIndexI18n;

	// Cultural names: We must store all data here, and have even 4 indices to native names&spelling, pronunciation, english and user-language spelling
	static QHash<StarId, QList<StelObject::CulturalName>> culturalNamesMap; // cultural names
	static QMap<QString, QList<StarId>>  culturalNativeNamesIndex; // reverse mappings. For names, unfortunately multiple results are possible!
	static QMap<QString, QList<StarId>>  culturalPronounceIndex;
	static QMap<QString, QList<StarId>>  culturalPronounceI18nIndex;

	static QHash<StarId, QString> sciDesignationsMapI18n;
	static QMap<QString, StarId> sciDesignationsIndexI18n;
	static QHash<StarId, QString> sciExtraDesignationsMapI18n;
	static QMap<QString, StarId> sciExtraDesignationsIndexI18n;

	static QHash<StarId, varstar> varStarsMapI18n;
	static QMap<QString, StarId> varStarsIndexI18n;

	static QHash<StarId, wds> wdsStarsMapI18n;
	static QMap<QString, StarId> wdsStarsIndexI18n;

	static QMap<QString, crossid> crossIdMap;
	static QHash<int, StarId> saoStarsIndex;
	static QHash<int, StarId> hdStarsIndex;
	static QHash<int, StarId> hrStarsIndex;

	static QHash<StarId, QString> referenceMap;

	static QHash<StarId, binaryorbitstar> binaryOrbitStarMap;

	QFont starFont;
	static bool flagSciNames;
	static bool flagAdditionalStarNames;
	static bool flagDesignations;
	static bool flagDblStarsDesignation;
	static bool flagVarStarsDesignation;
	static bool flagHIPDesignation;

	StelTextureSP texPointer;		// The selection pointer texture

	class StelObjectMgr* objectMgr;

	QString starConfigFileFullPath;
	QVariantMap starSettings;
	QVariantList catalogsDescription;
};


#endif // STARMGR_HPP

