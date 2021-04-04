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
#include "StelProjectorType.hpp"

class StelObject;
class StelToneReproducer;
class StelProjector;
class StelPainter;
class QSettings;

class ZoneArray;
struct HipIndexStruct;

static const int RCMAG_TABLE_SIZE = 4096;

typedef struct
{
	QString designation;	//! GCVS designation
	QString vtype;		//! Type of variability
	float maxmag;		//! Magnitude at maximum brightness
	int mflag;		//! Magnitude flag code
	float min1mag;		//! First minimum magnitude or amplitude
	float min2mag;		//! Second minimum magnitude or amplitude
	QString photosys;	//! The photometric system for magnitudes
	double epoch;		//! Epoch for maximum light (Julian days)
	double period;		//! Period of the variable star (days)
	int Mm;			//! Rising time or duration of eclipse (%)
	QString stype;		//! Spectral type
} varstar;

typedef struct
{
	QString designation;	//! WDS designation
	int observation;	//! Date of last satisfactory observation, yr
	float positionAngle;	//! Position Angle at date of last satisfactory observation, deg
	float separation;	//! Separation at date of last satisfactory observation, arcsec
} wds;

typedef struct
{
	int sao;
	int hd;
	int hr;
} crossid;

typedef QMap<StelObjectP, float> StelACStarData;

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
	//! - Loads the texture of the star selection indicator
	//! - Sets various display flags from the ini parser object
	virtual void init();

	//! Draw the stars and the star selection indicator if necessary.
	virtual void draw(StelCore* core);

	//! Update any time-dependent features.
	//! Includes fading in and out stars and labels when they are turned on and off.
	virtual void update(double deltaTime) {labelsFader.update(static_cast<int>(deltaTime*1000)); starsFader.update(static_cast<int>(deltaTime*1000));}

	//! Used to determine the order in which the various StelModules are drawn.
	virtual double getCallOrder(StelModuleActionName actionName) const;

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in StelObjectModule class
	//! Return a list containing the stars located inside the limFov circle around position v
	virtual QList<StelObjectP > searchAround(const Vec3d& v, double limitFov, const StelCore* core) const;

	//! Return the matching Stars object's pointer if exists or Q_NULLPTR
	//! @param nameI18n The case in-sensitive localized star common name or HIP/HP, SAO, HD, HR, GCVS or WDS number
	//! catalog name (format can be HP1234 or HP 1234 or HIP 1234) or sci name
	virtual StelObjectP searchByNameI18n(const QString& nameI18n) const;

	//! Return the matching star if exists or Q_NULLPTR
	//! @param name The case in-sensitive english star name
	virtual StelObjectP searchByName(const QString& name) const;

	//! Same as searchByName(id);
	virtual StelObjectP searchByID(const QString &id) const;

	//! Find and return the list of at most maxNbItem objects auto-completing the passed object English name.
	//! @param objPrefix the case insensitive first letters of the searched object
	//! @param maxNbItem the maximum number of returned object names
	//! @param useStartOfWords the autofill mode for returned objects names
	//! @return a list of matching object name by order of relevance, or an empty list if nothing match
	virtual QStringList listMatchingObjects(const QString& objPrefix, int maxNbItem=5, bool useStartOfWords=false) const;
	//! @note Loading stars with the common names only.
	virtual QStringList listAllObjects(bool inEnglish) const;	
	virtual QStringList listAllObjectsByType(const QString& objType, bool inEnglish) const;
	virtual QString getName() const { return "Stars"; }
	virtual QString getStelObjectType() const;

public slots:
	///////////////////////////////////////////////////////////////////////////
	// Methods callable from script and GUI
	//! Set display flag for Stars.
	void setFlagStars(bool b) {starsFader=b; emit starsDisplayedChanged(b);}
	//! Get display flag for Stars
	bool getFlagStars(void) const {return starsFader==true;}

	//! Set display flag for Star names (labels).
	void setFlagLabels(bool b) {labelsFader=b; emit starLabelsDisplayedChanged(b);}
	//! Get display flag for Star names (labels).
	bool getFlagLabels(void) const {return labelsFader==true;}

	//! Set the amount of star labels. The real amount is also proportional with FOV.
	//! The limit is set in function of the stars magnitude
	//! @param a the amount between 0 and 10. 0 is no labels, 10 is maximum of labels
	void setLabelsAmount(double a) {if(a!=labelsAmount){ labelsAmount=a; emit labelsAmountChanged(a);}}
	//! Get the amount of star labels. The real amount is also proportional with FOV.
	//! @return the amount between 0 and 10. 0 is no labels, 10 is maximum of labels
	double getLabelsAmount(void) const {return labelsAmount;}

	//! Define font size to use for star names display.
	void setFontSize(int newFontSize);

	//! Show scientific or catalog names on stars without common names.
	static void setFlagSciNames(bool f) {flagSciNames = f;}
	static bool getFlagSciNames(void) {return flagSciNames;}

	//! Set flag for usage designations of stars for their labels instead common names.
	void setDesignationUsage(const bool flag) { if(flagDesignations!=flag){ flagDesignations=flag; emit designationUsageChanged(flag);}}
	//! Get flag for usage designations of stars for their labels instead common names.
	bool getDesignationUsage(void) {return flagDesignations; }

	//! Show additional star names.
	void setFlagAdditionalNames(bool flag) { if (flagAdditionalStarNames!=flag){ flagAdditionalStarNames=flag; emit flagAdditionalNamesDisplayedChanged(flag);}}
	static bool getFlagAdditionalNames(void) { return flagAdditionalStarNames; }

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
	//! @param hip The Hipparcos number of star
	//! @return translated common name of star
	static QString getCommonName(int hip);

	//! Get the (translated) scientific name for a star with a specified
	//! Hipparcos catalogue number.
	//! @param hip The Hipparcos number of star
	//! @return translated scientific name of star
	static QString getSciName(int hip);

	//! Get the (translated) additional scientific name for a star with a
	//! specified Hipparcos catalogue number.
	//! @param hip The Hipparcos number of star
	//! @return translated additional scientific name of star
	static QString getSciAdditionalName(int hip);

	//! Get the (translated) additional scientific name for a star with a
	//! specified Hipparcos catalogue number.
	//! @param hip The Hipparcos number of star
	//! @return translated additional scientific name of star
	static QString getSciAdditionalDblName(int hip);

	//! Get the (translated) scientific name for a variable star with a specified
	//! Hipparcos catalogue number.
	//! @param hip The Hipparcos number of star
	//! @return translated scientific name of variable star
	static QString getGcvsName(int hip);

	//! Get the (translated) scientific name for a double star with a specified
	//! Hipparcos catalogue number.
	//! @param hip The Hipparcos number of star
	//! @return translated scientific name of double star
	static QString getWdsName(int hip);

	//! Get the (English) common name for a star with a specified
	//! Hipparcos catalogue number.
	//! @param hip The Hipparcos number of star
	//! @return common name of star (from skyculture file star_names.fab)
	static QString getCommonEnglishName(int hip);

	//! Get the (translated) additional names for a star with a specified
	//! Hipparcos catalogue number.
	//! @param hip The Hipparcos number of star
	//! @return translated additional names of star
	static QString getAdditionalNames(int hip);

	//! Get the English additional names for a star with a specified
	//! Hipparcos catalogue number.
	//! @param hip The Hipparcos number of star
	//! @return additional names of star
	static QString getAdditionalEnglishNames(int hip);

	//! Get the cross-identification designations for a star with a specified
	//! Hipparcos catalogue number.
	//! @param hip The Hipparcos number of star
	//! @return cross-identification data
	static QString getCrossIdentificationDesignations(QString hip);

	//! Get the type of variability for a variable star with a specified
	//! Hipparcos catalogue number.
	//! @param hip The Hipparcos number of star
	//! @return type of variability
	static QString getGcvsVariabilityType(int hip);

	//! Get the magnitude at maximum brightness for a variable star with a specified
	//! Hipparcos catalogue number.
	//! @param hip The Hipparcos number of star
	//! @return the magnitude at maximum brightness for a variable star
	static float getGcvsMaxMagnitude(int hip);

	//! Get the magnitude flag code for a variable star with a specified
	//! Hipparcos catalogue number.
	//! @param hip The Hipparcos number of star
	//! @return the magnitude flag code for a variable star
	static int getGcvsMagnitudeFlag(int hip);

	//! Get the minimum magnitude or amplitude for a variable star with a specified
	//! Hipparcos catalogue number.
	//! @param hip The Hipparcos number of star
	//! @param firstMinimumFlag
	//! @return the minimum magnitude or amplitude for a variable star
	static float getGcvsMinMagnitude(int hip, bool firstMinimumFlag=true);

	//! Get the photometric system for a variable star with a specified
	//! Hipparcos catalogue number.
	//! @param hip The Hipparcos number of star
	//! @return the photometric system for a variable star
	static QString getGcvsPhotometricSystem(int hip);

	//! Get Epoch for maximum light for a variable star with a specified
	//! Hipparcos catalogue number.
	//! @param hip The Hipparcos number of star
	//! @return Epoch for maximum light for a variable star
	static double getGcvsEpoch(int hip);

	//! Get the period for a variable star with a specified
	//! Hipparcos catalogue number.
	//! @param hip The Hipparcos number of star
	//! @return the period of variable star
	static double getGcvsPeriod(int hip);

	//! Get the rising time or duration of eclipse for a variable star with a
	//! specified Hipparcos catalogue number.
	//! @param hip The Hipparcos number of star
	//! @return the rising time or duration of eclipse for variable star
	static int getGcvsMM(int hip);

	//! Get year of last satisfactory observation of double star with a
	//! Hipparcos catalogue number.
	//! @param hip The Hipparcos number of star
	//! @return year of last satisfactory observation
	static int getWdsLastObservation(int hip);

	//! Get position angle at date of last satisfactory observation of double star with a
	//! Hipparcos catalogue number.
	//! @param hip The Hipparcos number of star
	//! @return position angle in degrees
	static float getWdsLastPositionAngle(int hip);

	//! Get separation angle at date of last satisfactory observation of double star with a
	//! Hipparcos catalogue number.
	//! @param hip The Hipparcos number of star
	//! @return separation in arcseconds
	static float getWdsLastSeparation(int hip);

	//! Get the parallax error for star with a Hipparcos catalogue number.
	//! @param hip The Hipparcos number of star
	//! @return the parallax error (mas)
	static float getPlxError(int hip);

	static QString convertToSpectralType(int index);
	static QString convertToComponentIds(int index);

	QVariantList getCatalogsDescription() const {return catalogsDescription;}

	//! Try to load the given catalog, even if it is marched as unchecked.
	//! Mark it as checked if checksum is correct.
	//! @return false in case of failure.
	bool checkAndLoadCatalog(const QVariantMap& m);

	//! Get the list of all Hipparcos stars.
	const QList<StelObjectP>& getHipparcosStars() const { return hipparcosStars; }	
	const QList<QMap<StelObjectP, float>>& getHipparcosHighPMStars() const { return hipStarsHighPM; }
	const QList<QMap<StelObjectP, float>>& getHipparcosDoubleStars() const { return doubleHipStars; }
	const QList<QMap<StelObjectP, float>>& getHipparcosVariableStars() const { return variableHipStars; }
	const QList<QMap<StelObjectP, float>>& getHipparcosAlgolTypeStars() const { return algolTypeStars; }
	const QList<QMap<StelObjectP, float>>& getHipparcosClassicalCepheidsTypeStars() const { return classicalCepheidsTypeStars; }

private slots:
	//! Translate text.
	void updateI18n();

	//! Called when the sky culture is updated.
	//! Loads common and scientific names of stars for a given sky culture.
	//! @param skyCultureDir the name of the directory containing the sky culture to use.
	void updateSkyCulture(const QString& skyCultureDir);

	//! increase artificial cutoff magnitude slightly (can be linked to an action/hotkey)
	void increaseStarsMagnitudeLimit();
	//! decrease artificial cutoff magnitude slightly (can be linked to an action/hotkey)
	void reduceStarsMagnitudeLimit();

signals:
	void starLabelsDisplayedChanged(const bool displayed);
	void starsDisplayedChanged(const bool displayed);
	void designationUsageChanged(const bool flag);
	void flagAdditionalNamesDisplayedChanged(const bool displayed);
	void labelsAmountChanged(float a);

private:
	void setCheckFlag(const QString& catalogId, bool b);

	void copyDefaultConfigFile();

	//! Loads common names for stars from a file.
	//! Called when the SkyCulture is updated.
	//! @param the path to a file containing the common names for bright stars.
	//! @note Stellarium doesn't support sky cultures made prior version 0.10.6 now!
	int loadCommonNames(const QString& commonNameFile);

	//! Loads scientific names for stars from a file.
	//! Called when the SkyCulture is updated.
	//! @param the path to a file containing the scientific names for bright stars.
	void loadSciNames(const QString& sciNameFile);

	//! Loads GCVS from a file.
	//! @param the path to a file containing the GCVS.
	void loadGcvs(const QString& GcvsFile);

	//! Loads WDS from a file.
	//! @param the path to a file containing the WDS.
	void loadWds(const QString& WdsFile);

	//! Loads cross-identification data from a file.
	//! @param the path to a file containing the cross-identification data.
	void loadCrossIdentificationData(const QString& crossIdFile);

	//! Loads parallax error data from a file.
	//! @param the path to a file containing the parallax error data.
	void loadPlxErr(const QString& plxErrFile);

	//! Gets the maximum search level.
	// TODO: add a non-lame description - what is the purpose of the max search level?
	int getMaxSearchLevel() const;

	//! Load all the stars from the files.
	void loadData(QVariantMap starsConfigFile);

	//! Draw a nice animated pointer around the object.
	void drawPointer(StelPainter& sPainter, const StelCore* core);

	void populateHipparcosLists();
	void populateStarsDesignations();

	//! List of all Hipparcos stars.
	QList<StelObjectP> hipparcosStars;
	QList<QMap<StelObjectP, float>> doubleHipStars, variableHipStars, algolTypeStars, classicalCepheidsTypeStars, hipStarsHighPM;

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

	static QHash<int, QString> commonNamesMap;     // the original names from skyculture (star_names.fab)
	static QHash<int, QString> commonNamesMapI18n; // translated names
	static QMap<QString, int> commonNamesIndexI18n;
	static QMap<QString, int> commonNamesIndex;

	static QHash<int, QString> additionalNamesMap; // additional names
	static QHash<int, QString> additionalNamesMapI18n;
	static QMap<QString, int> additionalNamesIndex;
	static QMap<QString, int> additionalNamesIndexI18n;

	static QHash<int, QString> sciNamesMapI18n;	
	static QMap<QString, int> sciNamesIndexI18n;

	static QHash<int, QString> sciAdditionalNamesMapI18n;
	static QMap<QString, int> sciAdditionalNamesIndexI18n;

	static QHash<int, QString> sciAdditionalDblNamesMapI18n;
	static QMap<QString, int> sciAdditionalDblNamesIndexI18n;

	static QHash<int, varstar> varStarsMapI18n;
	static QMap<QString, int> varStarsIndexI18n;

	static QHash<int, wds> wdsStarsMapI18n;
	static QMap<QString, int> wdsStarsIndexI18n;

	static QMap<QString, crossid> crossIdMap;
	static QMap<int, int> saoStarsIndex;
	static QMap<int, int> hdStarsIndex;
	static QMap<int, int> hrStarsIndex;

	static QHash<int, float> hipParallaxErrors;

	static QHash<int, QString> referenceMap;

	QFont starFont;
	static bool flagSciNames;
	static bool flagAdditionalStarNames;
	static bool flagDesignations;

	StelTextureSP texPointer;		// The selection pointer texture

	class StelObjectMgr* objectMgr;

	QString starConfigFileFullPath;
	QVariantMap starSettings;
	QVariantList catalogsDescription;
};


#endif // STARMGR_HPP

