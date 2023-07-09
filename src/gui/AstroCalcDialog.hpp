/*
 * Stellarium
 * 
 * Copyright (C) 2015 Alexander Wolf
 * Copyright (C) 2016 Nick Fedoseev (visualization of ephemeris)
 * Copyright (C) 2022 Georg Zotti
 * Copyright (C) 2022 Worachate Boonplod (Eclipses)
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
*/

#ifndef ASTROCALCDIALOG_HPP
#define ASTROCALCDIALOG_HPP

#include <QObject>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QMap>
#include <QVector>
#include <QTimer>
#include <QRegularExpression>
#include <QMutex>
#include <QtCharts/qchartview.h>

#include "AstroCalcChart.hpp"
#include "StelDialog.hpp"
#include "StelCore.hpp"
#include "Planet.hpp"
#include "SolarSystem.hpp"
#include "NebulaMgr.hpp"
#include "StelPropertyMgr.hpp"
#include "StarMgr.hpp"
#include "StelUtils.hpp"

class Ui_astroCalcDialogForm;
class QListWidgetItem;
class QSortFilterProxyModel;
class QStringListModel;
class AstroCalcExtraEphemerisDialog;
class AstroCalcCustomStepsDialog;

struct Ephemeris
{
	Vec3d coord;
	Vec3d sunCoord; // We need that to compute comet tail icon directions in projected screen space.
	int colorIndex;
	double objDate;
	QString objDateStr;
	float magnitude;
	bool isComet;
	PlanetP sso;
};
Q_DECLARE_METATYPE(Ephemeris)

// Intermediate data structure for the simple solar system map.
struct HECPosition
{
	QString objectName;
	double angle; // derived from heliocentric ecl. longitude
	double dist;  // derived from helioc. distance
};
Q_DECLARE_METATYPE(HECPosition)

class AstroCalcDialog : public StelDialog
{
	Q_OBJECT

public:
	//! Defines the number and the order of the columns in the table that lists celestial bodies positions
	//! @enum CPositionsColumns
	enum CPositionsColumns {
		CColumnName,            //! name of object
		CColumnRA,              //! right ascension
		CColumnDec,             //! declination
		CColumnMagnitude,       //! magnitude
		CColumnAngularSize,     //! angular size
		CColumnExtra,           //! extra data (surface brightness, separation, period, etc.)
		CColumnTransit,		//! time of transit
		CColumnMaxElevation,    //! max. elevation
		CColumnElongation,      //! elongation (from the Sun)
		CColumnType,            //! type of object
		CColumnCount            //! total number of columns
	};

	//! Defines the number and the order of the columns in the table that major planets positions
	//! (heliocentric ecliptic coordinates)
	//! @enum HECPositionsColumns
	enum HECPositionsColumns {
		HECColumnName,          //! name of the planet
		HECColumnSymbol,        //! symbol of the planet
		HECColumnLatitude,      //! heliocentric ecliptical latitude
		HECColumnLongitude,     //! heliocentric ecliptical longitude
		HECColumnDistance,      //! distance
		HECColumnCount          //! total number of columns
	};

	//! Defines the number and the order of the columns in the ephemeris table
	//! @enum EphemerisColumns
	enum EphemerisColumns {
		EphemerisCOName,        //! name of celestial object
		EphemerisDate,          //! date and time of ephemeris
		EphemerisRA,            //! right ascension
		EphemerisDec,           //! declination
		EphemerisMagnitude,     //! magnitude
		EphemerisPhase,         //! phase
		EphemerisDistance,      //! distance
		EphemerisElongation,    //! elongation
		EphemerisCount          //! total number of columns
	};

	//! Defines the number and the order of the columns in the rises, transits and sets table
	//! @enum RTSColumns
	enum RTSColumns {
		RTSCOName,              //! name of celestial object
		RTSRiseDate,            //! date and time of rise
		RTSTransitDate,         //! date and time of transit
		RTSSetDate,             //! date and time of set
		RTSTransitAltitude,     //! altitude at transit
		RTSMagnitude,           //! magnitude at transit
		RTSElongation,          //! elongation at transit (from the Sun)
		RTSAngularDistance,     //! angular distance at transit (from the Moon)
		RTSCount                //! total number of columns
	};

	//! Defines the number and the order of the columns in the phenomena table
	//! @enum PhenomenaColumns
	enum PhenomenaColumns {
		PhenomenaType,          //! type of phenomena
		PhenomenaDate,          //! date and time of ephemeris
		PhenomenaObject1,       //! first object
		PhenomenaMagnitude1,    //! magnitude of first object
		PhenomenaObject2,       //! second object
		PhenomenaMagnitude2,    //! magnitude of second object
		PhenomenaSeparation,    //! angular separation
		PhenomenaElevation,     //! elevation of first object
		PhenomenaElongation,    //! elongation (from the Sun)
		PhenomenaAngularDistance, //! angular distance (from the Moon)
		PhenomenaCount          //! total number of columns
	};

	enum PhenomenaTypeIndex {
		Conjunction		= 0,
		Opposition		= 1,
		GreatestElongation	= 2,
		StationaryPoint		= 3,
		OrbitalPoint		= 4,
		Shadows			= 5,
		Quadrature		= 6
	};

	//! Defines the number and the order of the columns in the WUT tool
	//! @enum WUTColumns
	enum WUTColumns {
		WUTObjectName,          //! object name
		WUTMagnitude,           //! magnitude
		WUTRiseTime,            //! rise time
		WUTTransitTime,         //! transit time
		WUTMaxElevation,        //! max. elevation
		WUTSetTime,             //! set time
		WUTAngularSize,         //! angular size
		WUTConstellation,       //! IAU constellation
		WUTObjectType,	        //! object type
		WUTCount                //! total number of columns
	};

	//! Defines the number and the order of the columns in the lunar eclipse table
	//! @enum LunarEclipseColumns
	enum LunarEclipseColumns {
		LunarEclipseDate,		//! date and time of lunar eclipse
		LunarEclipseSaros,		//! Saros number
		LunarEclipseType,		//! type of lunar eclipse
		LunarEclipseGamma,		//! Gamma of lunar eclipse
		LunarEclipsePMag,		//! penumbral magnitude of lunar eclipse
		LunarEclipseUMag,		//! umbral magnitude of lunar eclipse
		LunarEclipseVisConditions,	//! visibility conditions
		LunarEclipseCount		//! total number of columns
	};

	//! Defines the number and the order of the columns in the lunar eclipse contact table
	//! @enum LunarEclipseContactColumns
	enum LunarEclipseContactColumns {
		LunarEclipseContact,		//! circumstance of lunar eclipse
		LunarEclipseContactDate,	//! date and time of circumstance
		LunarEclipseContactAltitude,	//! altitude of the Moon
		LunarEclipseContactAzimuth,	//! azimuth of the Moon
		LunarEclipseContactLatitude,	//! latitude where the Moon appears in the zenith
		LunarEclipseContactLongitude,	//! longitude where the Moon appears in the zenith
		LunarEclipseContactPA,		//! position angle of shadow
		LunarEclipseContactDistance,	//! angular distance between the Moon and shadow
		LunarEclipseContactCount	//! total number of columns
	};

	//! Defines the number and the order of the columns in the global solar eclipse table
	//! @enum SolarEclipseColumns
	enum SolarEclipseColumns {
		SolarEclipseDate,		//! date and time of solar eclipse
		SolarEclipseSaros,		//! saros number of solar eclipse
		SolarEclipseType,		//! type of solar eclipse
		SolarEclipseGamma,		//! gamma of solar eclipse
		SolarEclipseMag,		//! greatest magnitude of solar eclipse
		SolarEclipseLatitude,		//! latitude at greatest eclipse
		SolarEclipseLongitude,	//! longitude at greatest eclipse
		SolarEclipseAltitude,		//! altitude of the Sun at greatest eclipse
		SolarEclipsePathwidth,	//! pathwidth of total or annular solar eclipse
		SolarEclipseDuration,		//! central duration of total or annular solar eclipse
		SolarEclipseCount		//! total number of columns
	};

	//! Defines the number and the order of the columns in the global solar eclipse contact table
	//! @enum SolarEclipseContactColumns
	enum SolarEclipseContactColumns {
		SolarEclipseContact,		//! circumstance of solar eclipse
		SolarEclipseContactDate,	//! date and time of circumstance
		SolarEclipseContactLatitude,	//! latitude at contact time
		SolarEclipseContactLongitude,	//! longitude at contact time
		SolarEclipseContactPathwidth,	//! pathwidth of total or annular solar eclipse
		SolarEclipseContactDuration,	//! central duration of total or annular solar eclipse
		SolarEclipseContactType,	//! type of solar eclipse		
		SolarEclipseContactCount	//! total number of columns
	};

	//! Defines the number and the order of the columns in the local solar eclipse table
	//! @enum SolarEclipseColumns
	enum SolarEclipseLocalColumns {
		SolarEclipseLocalDate,		//! date of maximum solar eclipse
		SolarEclipseLocalType,		//! type of solar eclipse
		SolarEclipseLocalFirstContact,	//! time of the beginning of partial solar eclipse
		SolarEclipseLocal2ndContact,	//! time of the beginning of total/annular solar eclipse
		SolarEclipseLocalMaximum,		//! time of maximum solar eclipse
		SolarEclipseLocalMagnitude,	//! maximum magnitude of solar eclipse
		SolarEclipseLocal3rdContact,	//! time of the end of total/annular solar eclipse
		SolarEclipseLocalLastContact,	//! time of the end of partial solar eclipse
		SolarEclipseLocalDuration,		//! duration of total/annular solar eclipse
		SolarEclipseLocalCount		//! total number of columns
	};

	//! Defines the number and the order of the columns in transit table
	//! @enum TransitColumns
	enum TransitColumns {
		TransitDate,			//! date of mid-transit
		TransitPlanet,			//! transit planet
		TransitContact1,		//! time of exterior ingress
		TransitContact2,		//! time of interior ingress
		TransitMid,			//! time of mid-transit
		TransitSeparation,		//! minimum angular distance to Sun's center
		TransitContact3,		//! time of interior egress
		TransitContact4,		//! time of exterior egress
		TransitDuration,		//! duration of transit
		TransitObservableDuration,	//! observable duration of transit
		TransitCount			//! total number of columns
	};

	AstroCalcDialog(QObject* parent);
	~AstroCalcDialog() override;

	static QVector<Ephemeris> EphemerisList;
	static int DisplayedPositionIndex;


public slots:
	void retranslate() override;
		
signals:
	//! This signal is emitted when the graph day changed.
	void graphDayChanged();

protected:
        //! Initialize the dialog widgets and connect the signals/slots.
	void createDialogContent() override;
        Ui_astroCalcDialogForm *ui;

private slots:
	void currentCelestialPositions();
	void populateCelestialCategoryList();
	void saveCelestialPositions();
	void selectCurrentCelestialPosition(const QModelIndex &modelIndex);

	void currentHECPositions();
	void drawHECGraph(const QString &selectedObject = "");
	void saveHECPositions();
	void selectCurrentHECPosition(const QModelIndex &modelIndex);
	void markCurrentHECPosition(const QModelIndex &modelIndex);
	void saveHECFlagMinorPlanets(bool b);
	void saveHECFlagBrightComets(bool b);
	void saveHECBrightCometMagnitudeLimit(double mag);

	void saveCelestialPositionsMagnitudeLimit(double mag);
	void saveCelestialPositionsHorizontalCoordinatesFlag(bool b);
	void saveCelestialPositionsCategory(int index);

	//! Calculating ephemeris for selected celestial body and fill the list.
	void generateEphemeris();
	void cleanupEphemeris();
	void selectCurrentEphemeride(const QModelIndex &modelIndex);
	void saveEphemeris();	
	void onChangedEphemerisPosition();
	void reGenerateEphemeris();
	void setDateTimeNow();
	void saveIgnoreDateTestFlag(bool b);

	//! Calculating the rises, transits and sets for selected celestial body and fill the list.
	void generateRTS();
	void cleanupRTS();
	void selectCurrentRTS(const QModelIndex &modelIndex);
	void saveRTS();
	void setRTSCelestialBodyName();

	//! Calculating lunar eclipses to fill the list.
	//! Algorithm taken from calculating the rises, transits and sets.
	void generateLunarEclipses();
	void cleanupLunarEclipses();
	void selectCurrentLunarEclipse(const QModelIndex &modelIndex);
	void selectCurrentLunarEclipseDate(const QModelIndex &modelIndex);
	void selectCurrentLunarEclipseContact(const QModelIndex &modelIndex);
	void saveLunarEclipses();
	void saveLunarEclipseCircumstances();

	//! Calculating solar eclipses to fill the list.
	//! Algorithm taken from calculating the rises, transits and sets.
	void generateSolarEclipses();
	void cleanupSolarEclipses();
	void selectCurrentSolarEclipse(const QModelIndex &modelIndex);
	void selectCurrentSolarEclipseDate(const QModelIndex &modelIndex);
	void selectCurrentSolarEclipseContact(const QModelIndex &modelIndex);
	void saveSolarEclipses();
	void saveSolarEclipseCircumstances();
	void saveSolarEclipseKML();

	//! Calculating local solar eclipses to fill the list.
	//! Algorithm taken from calculating the rises, transits and sets.
	void generateSolarEclipsesLocal();
	void cleanupSolarEclipsesLocal();
	void selectCurrentSolarEclipseLocal(const QModelIndex &modelIndex);
	void saveSolarEclipsesLocal();

	//! Calculating transits to fill the list.
	//! Algorithm taken from calculating the rises, transits and sets.
	void generateTransits();
	void cleanupTransits();
	void selectCurrentTransit(const QModelIndex &modelIndex);
	void saveTransits();

	void saveEphemerisCelestialBody(int index);
	void saveEphemerisSecondaryCelestialBody(int index);
	void saveEphemerisTimeStep(int index);
	void initEphemerisFlagNakedEyePlanets(void);
	void saveEphemerisFlagNakedEyePlanets(bool flag);

	//! Calculating phenomena for selected celestial body and fill the list.
	void calculatePhenomena();
	void cleanupPhenomena();
	void selectCurrentPhenomen(const QModelIndex &modelIndex);
	void savePhenomena();
	void savePhenomenaAngularSeparation();
	//! Populates the drop-down list of groups of celestial bodies.
	void populateGroupCelestialBodyList();

	void savePhenomenaCelestialBody(int index);
	void savePhenomenaCelestialGroup(int index);
	void savePhenomenaOppositionFlag(bool b);
	void savePhenomenaPerihelionAphelionFlag(bool b);
	void savePhenomenaElongationsQuadraturesFlag(bool b);

	//! Compute planetary data
	void saveFirstCelestialBody(int index);
	void saveSecondCelestialBody(int index);
	void computePlanetaryData();
	void drawDistanceGraph();

	void drawLunarElongationGraph();
	void drawLunarElongationLimitLine();
	void saveLunarElongationLimit(int limit);

	//! Draw diagram 'Altitude vs. Time'
	void drawAltVsTimeDiagram();
	//! Draw vertical line 'Now' on diagram 'Altitude vs. Time'
	void drawCurrentTimeDiagram();
	void saveAltVsTimeSunFlag(bool state);
	void saveAltVsTimeMoonFlag(bool state);
	void saveAltVsTimePositiveFlag(bool state);
	void saveAltVsTimePositiveLimit(int limit);

	//! Draw diagram 'Azimuth vs. Time'
	void drawAziVsTimeDiagram();

	//! handle events that are otherwise "lost" when dialog not visible
	void handleVisibleEnabled();

	void saveGraphsCelestialBody(int index);
	void saveGraphsFirstId(int index);
	void saveGraphsSecondId(int index);
	void updateGraphsDuration(int duration);
	void updateGraphsStep(int step);
	void drawXVsTimeGraphs();
	void updateXVsTimeGraphs();

	void drawMonthlyElevationGraph();
	void updateMonthlyElevationTime();
	void syncMonthlyElevationTime();
	void saveMonthlyElevationPositiveFlag(bool state);
	void saveMonthlyElevationPositiveLimit(int limit);

	// WUT
	void saveWutMagnitudeLimit(double mag);
	void saveWutMinAngularSizeLimit();
	void saveWutMaxAngularSizeLimit();
	void saveWutMinAltitude();
	void saveWutAngularSizeFlag(bool state);
	void saveWutTimeInterval(int index);
	void calculateWutObjects();
	void selectWutObject(const QModelIndex& index);
	void saveWutObjects();
	void searchWutClear();
	//! Populates the list of groups for WUT tool.
	void populateWutGroups();

	void updateAstroCalcData();

	void changePage(QListWidgetItem *current, QListWidgetItem *previous);
	void changePCTab(int index);
	void changeGraphsTab(int index);
	void changeEclipsesTab(int index);
	void changePositionsTab(int index);

	void updateSolarSystemData();
	void populateCelestialNames(QString);
	void showExtraEphemerisDialog();
	void showCustomStepsDialog();

	void saveGraph(QChartView *graph);

private:
	class AstroCalcExtraEphemerisDialog* extraEphemerisDialog;
	class AstroCalcCustomStepsDialog* customStepsDialog;
	class StelCore* core;
	class SolarSystem* solarSystem;
	class NebulaMgr* dsoMgr;
	class StarMgr* starMgr;
	class StelObjectMgr* objectMgr;
	class StelLocaleMgr* localeMgr;
	class StelMovementMgr* mvMgr;
	class StelPropertyMgr* propMgr;
	//QStringListModel* wutModel;
	//QSortFilterProxyModel *proxyModel;
	AstroCalcChart *altVsTimeChart;
	mutable QMutex altVsTimeChartMutex;
	AstroCalcChart *azVsTimeChart;
	mutable QMutex azVsTimeChartMutex;
	AstroCalcChart *monthlyElevationChart;
	mutable QMutex monthlyElevationChartMutex;
	AstroCalcChart *curvesChart;
	mutable QMutex curvesChartMutex;
	AstroCalcChart *lunarElongationChart;
	mutable QMutex lunarElongationChartMutex;
	AstroCalcChart *pcChart;
	mutable QMutex pcChartMutex;

	QSettings* conf;
	QTimer *currentTimeLine;
	QHash<QString,int> wutCategories;
	QList<HECPosition> hecObjects;

	void saveTableAsCSV(const QString& fileName, QTreeWidget* tWidget, QStringList& headers);
	void saveTableAsXLSX(const QString& fileName, QTreeWidget* tWidget, QStringList& headers, const QString& title, const QString& sheetName, const QString &note = "");
	void saveTableAsBookmarks(const QString& fileName, QTreeWidget* tWidget);
	QPair<QString, QString> askTableFilePath(const QString& caption, const QString& fileName);

	void populateToolTips();
	//! Get the list of selected dwarf and minor planets
	QList<PlanetP> getSelectedMinorPlanets();

	//! Update header names for celestial positions tables
	void setCelestialPositionsHeaderNames();
	//! Update header names for celestial positions tables (major planets; heliocentric ecliptic coordinates)
	void setHECPositionsHeaderNames();
	//! Update header names for ephemeris table
	void setEphemerisHeaderNames();
	//! update header names for rises, transists and sets table
	void setRTSHeaderNames();
	//! Update header names for phenomena table
	void setPhenomenaHeaderNames();
	//! Update header names for WUT table
	void setWUTHeaderNames(const bool magnitude = true, const bool separation = false);
	//! update header names for lunar eclipse table
	void setLunarEclipseHeaderNames();
	//! update header names for lunar eclipse contact table
	void setLunarEclipseContactsHeaderNames();
	//! update header names for solar eclipse table
	void setSolarEclipseHeaderNames();
	//! update header names for solar eclipse contact table
	void setSolarEclipseContactsHeaderNames();
	//! update header names for local solar eclipse table
	void setSolarEclipseLocalHeaderNames();
	//! update header names for transit table
	void setTransitHeaderNames();

	//! Init header and list of celestial positions
	void initListCelestialPositions();
	//! Init header and list of celestial positions (major planets; heliocentric ecliptic coordinates)
	void initListHECPositions();
	//! Init header and list of ephemeris
	void initListEphemeris();
	//! Init header and list of rises, transists and sets
	void initListRTS();
	//! Init header and list of phenomena
	void initListPhenomena();
	//! Init header and list of WUT
	void initListWUT(const bool magnitude = true, const bool separation = false);
	//! Init header and list of lunar eclipse
	void initListLunarEclipse();
	//! Init header and list of lunar eclipse contact
	void initListLunarEclipseContact();
	//! Init header and list of solar eclipse
	void initListSolarEclipse();
	//! Init header and list of solar eclipse contact
	void initListSolarEclipseContact();
	//! Iteration to calculate minimum distance from Besselian elements
	double getJDofMinimumDistance(double JD);
	//! Iteration to calculate JD of solar eclipse contacts
	double getJDofContact(double JD, bool beginning, bool penumbral, bool external, bool outerContact);
	//! Iteration to calculate contact times of solar eclipse
	double getDeltaTimeofContact(double JD, bool beginning, bool penumbra, bool external, bool outerContact);
	//! Geographic coordinates where solar eclipse begins/ends at sunrise/sunset
	QPair<double, double> getRiseSetLineCoordinates(bool first, double x, double y, double d, double L, double mu);
	//! Geographic coordinates where maximum solar eclipse occurs at sunrise/sunset
	QPair<double, double> getMaximumEclipseAtRiseSet(bool first, double JD);
	//! Geographic coordinates of shadow outline
	QPair<double, double> getShadowOutlineCoordinates(double angle, double x, double y, double d, double L, double tf,double mu);
	//! Geographic coordinates of northern and southern limit of shadow
	QPair<double, double> getNSLimitofShadow(double JD, bool northernLimit, bool penumbra);
	//! Geographic coordinates of extreme northern and southern limits of shadow
	QPair<double, double> getExtremeNSLimitofShadow(double JD, bool northernLimit, bool penumbra, bool begin);
	//! Geographic coordinates of extreme contact
	QPair<double, double> getContactCoordinates(double x, double y, double d, double mu);
	//! Init header and list of local solar eclipse
	void initListSolarEclipseLocal();
	//! Init header and list of transit
	void initListTransit();

	//! Populates the drop-down list of celestial bodies.
	//! The displayed names are localized in the current interface language.
	//! The original names are kept in the user data field of each QComboBox
	//! item.
	void populateCelestialBodyList();	
	//! Populates the drop-down list of time steps.
	void populateEphemerisTimeStepsList();
	//! Populates the drop-down list of planets.
	void populatePlanetList();
	//! Prepare graph settings
	void prepareAxesAndGraph();
	//! Populates the drop-down list of time intervals for WUT tool.
	void populateTimeIntervalsList();	
	double computeGraphValue(const PlanetP &ssObj, const AstroCalcChart::Series graphType);

	void populateFunctionsList();	
	double computeMaxElevation(StelObjectP obj);

	void adjustCelestialPositionsColumns();
	void adjustHECPositionsColumns();
	void adjustWUTColumns();
	void adjustPhenomenaColumns();

	void enableEphemerisButtons(bool enable);
	void enableRTSButtons(bool enable);
	void enablePhenomenaButtons(bool enable);
	void enableSolarEclipsesButtons(bool enable);
	void enableSolarEclipsesCircumstancesButtons(bool enable);
	void enableSolarEclipsesLocalButtons(bool enable);
	void enableLunarEclipsesButtons(bool enable);
	void enableLunarEclipsesCircumstancesButtons(bool enable);
	void enableTransitsButtons(bool enable);

	void enableCustomEphemerisTimeStepButton();
	double getCustomTimeStep();
	void reGenerateEphemeris(bool withSelection);

	//! Finding and selecting an object by its name in specific JD
	void goToObject(const QString &name, const double JD);

	//! Format RA/Dec or Az/Alt coordinates into nice strings.
	//! @arg horizontal coord are horizontal (alt-azimuthal). Use degrees/degrees. Else use Hours/degrees.
	//! @arg southAzimuth (relevant only for horizontal=true) count azimuth from south.
	//! @arg decimalDegrees use decimal format, not DMS/HMS
	//! @return QPair(lngStr, latStr) formatted output strings
	static QPair<QString, QString> getStringCoordinates(const Vec3d &coord, const bool horizontal, const bool southAzimuth, const bool decimalDegrees);
	void fillWUTTable(const QString &objectName, const QString &designation, float magnitude, const Vec4d &RTSTime, double maxElevation,
			  double angularSize, const QString &constellation, const QString &otype, bool decimalDegrees = false);
	void fillCelestialPositionTable(const QString &objectName, const QString &RA, const QString &Dec, double magnitude,
					const QString &angularSize, const QString &angularSizeToolTip, const QString &extraData,
					const QString &extraDataToolTip, const QString &transitTime, const QString &maxElevation,
					QString &sElongation, const QString &objectType);
	void fillHECPositionTable(const QString &objectName, const QChar objectSymbol, const QString &latitude, const QString &longitude, const double distance);

	//! Calculation conjunctions and oppositions.
	//! @note Ported from KStars, should be improved, because this feature calculates
	//! angular separation ("conjunction" defined as equality of right ascension
	//! of two bodies), and current solution is not accurate and slow.
	//! @note modes: 0 - conjunction, 1 - opposition, 2 - greatest elongation
	QMap<double, double> findClosestApproach(PlanetP& object1, StelObjectP& object2, const double startJD, const double stopJD, const double maxSeparation, const int mode);
	//! Finding the angular distance between two celestial bodies at some Julian date
	double findDistance(double JD, PlanetP object1, StelObjectP object2, int mode);
	//! Finding the initial time steps for interactions
	double findInitialStep(double startJD, double stopJD, QStringList &objects);
	//! Finding the celestial event
	bool findPrecise(QPair<double, double>* out, PlanetP object1, StelObjectP object2, double JD, double step, int prevSign, int mode);
	//! Wrapper for filling the table of phenomena between planet and star
	void fillPhenomenaTable(const QMap<double, double> list, const PlanetP object1, const StelObjectP object2, int mode);
	//! Wrapper for filling the table of phenomena between planet and deep-sky object
	void fillPhenomenaTable(const QMap<double, double> list, const PlanetP object1, const NebulaP object2);
	//! Wrapper for filling the table of phenomena between two planets
	//! @note modes: 0 - conjunction, 1 - opposition, 2 - greatest elongation
	void fillPhenomenaTable(const QMap<double, double> list, const PlanetP object1, const PlanetP object2, int mode);
	//! Filling the table of phenomena
	void fillPhenomenaTableVis(const QString &phenomenType, double JD, const QString &firstObjectName, float firstObjectMagnitude,
				   const QString &secondObjectName, float secondObjectMagnitude, const QString &separation, const QString &elevation,
				   QString &elongation, const QString &angularDistance, const QString &elongTooltip="", const QString &angDistTooltip="");
	//! Calculation of greatest elongations
	QMap<double, double> findGreatestElongationApproach(PlanetP& object1, StelObjectP& object2, double startJD, double stopJD);
	bool findPreciseGreatestElongation(QPair<double, double>* out, PlanetP object1, StelObjectP object2, double JD, double stopJD, double step);
	//! Calculation of quadratures
	QMap<double, double> findQuadratureApproach(PlanetP& object1, StelObjectP& object2, double startJD, double stopJD);
	bool findPreciseQuadrature(QPair<double, double>* out, PlanetP object1, StelObjectP object2, double JD, double stopJD, double step);
	//! Calculation of stationary points
	QMap<double, double> findStationaryPointApproach(PlanetP& object1, double startJD, double stopJD);
	bool findPreciseStationaryPoint(QPair<double, double>* out, PlanetP object, double JD, double stopJD, double step, bool retrograde);
	double findRightAscension(double JD, PlanetP object);
	//! Calculation of perihelion and aphelion points
	QMap<double, double> findOrbitalPointApproach(PlanetP& object1, double startJD, double stopJD);
	bool findPreciseOrbitalPoint(QPair<double, double>* out, PlanetP object1, double JD, double stopJD, double step, bool minimal);
	inline double findHeliocentricDistance(double JD, PlanetP object1) const {return object1->getHeliocentricEclipticPos(JD+core->computeDeltaT(JD)/86400.).norm();}
	bool isSecondObjectRight(double JD, PlanetP object1, StelObjectP object2);

	// Signal that a plot has to be redone
	bool plotAltVsTime, plotAltVsTimeSun, plotAltVsTimeMoon, plotAltVsTimePositive, plotMonthlyElevation, plotMonthlyElevationPositive, plotDistanceGraph, plotLunarElongationGraph, plotAziVsTime;
	int altVsTimePositiveLimit, monthlyElevationPositiveLimit, graphsDuration, graphsStep;
	QStringList ephemerisHeader, phenomenaHeader, positionsHeader, hecPositionsHeader, wutHeader, rtsHeader, lunareclipseHeader, lunareclipsecontactsHeader, solareclipseHeader, solareclipsecontactsHeader, solareclipselocalHeader, transitHeader;
	static double brightLimit;
	static const QString dash, delimiter;

	//! Make sure that no tabs icons are outside of the viewport.
	//! @todo Limit the width to the width of the screen *available to the window*.
	void updateTabBarListWidgetWidth();

	void enableAngularLimits(bool enable);

	QString getSelectedObjectNameI18n(StelObjectP selectedObject);

	//! Memorize day for detecting rollover to next/prev one
	int oldGraphJD;

	//! Remember to redraw active plot when dialog becomes visible
	bool graphPlotNeedsRefresh;

	enum PhenomenaCategory {
		PHCLatestSelectedObject     = -1,
		PHCSolarSystem              =  0,
		PHCPlanets                  =  1,
		PHCAsteroids                =  2,
		PHCPlutinos                 =  3,
		PHCComets                   =  4,
		PHCDwarfPlanets             =  5,
		PHCCubewanos                =  6,
		PHCScatteredDiscObjects     =  7,
		PHCOortCloudObjects         =  8,
		PHCSednoids                 =  9,
		PHCBrightStars              = 10,
		PHCBrightDoubleStars        = 11,
		PHCBrightVariableStars      = 12,
		PHCBrightStarClusters       = 13,
		PHCPlanetaryNebulae         = 14,
		PHCBrightNebulae            = 15,
		PHCDarkNebulae              = 16,
		PHCBrightGalaxies           = 17,
		PHCSymbioticStars           = 18,
		PHCEmissionLineStars        = 19,
		PHCInterstellarObjects      = 20,
		PHCPlanetsSun               = 21,
		PHCSunPlanetsMoons          = 22,
		PHCBrightSolarSystemObjects = 23,
		PHCSolarSystemMinorBodies   = 24,
		PHCMoonsFirstBody           = 25,
		PHCBrightCarbonStars        = 26,
		PHCBrightBariumStars        = 27,
		PHCSunPlanetsTheirMoons     = 28,
		PHCNone	// stop gapper for syntax reasons
	};

	enum WUTCategory {
		EWPlanets                            =  0,
		EWBrightStars                        =  1,
		EWBrightNebulae                      =  2,
		EWDarkNebulae                        =  3,
		EWGalaxies                           =  4,
		EWOpenStarClusters                   =  5,
		EWAsteroids                          =  6,
		EWComets                             =  7,
		EWPlutinos                           =  8,
		EWDwarfPlanets                       =  9,
		EWCubewanos                          = 10,
		EWScatteredDiscObjects               = 11,
		EWOortCloudObjects                   = 12,
		EWSednoids                           = 13,
		EWPlanetaryNebulae                   = 14,
		EWBrightDoubleStars                  = 15,
		EWBrightVariableStars                = 16,
		EWBrightStarsWithHighProperMotion    = 17,
		EWSymbioticStars                     = 18,
		EWEmissionLineStars                  = 19,
		EWSupernovaeCandidates               = 20,
		EWSupernovaeRemnantCandidates        = 21,
		EWSupernovaeRemnants                 = 22,
		EWClustersOfGalaxies                 = 23,
		EWInterstellarObjects                = 24,
		EWGlobularStarClusters               = 25,
		EWRegionsOfTheSky                    = 26,
		EWActiveGalaxies                     = 27,
		EWPulsars                            = 28,
		EWExoplanetarySystems                = 29,
		EWBrightNovaStars                    = 30,
		EWBrightSupernovaStars               = 31,
		EWInteractingGalaxies                = 32,
		EWDeepSkyObjects                     = 33,
		EWMessierObjects                     = 34,
		EWNGCICObjects                       = 35,
		EWCaldwellObjects                    = 36,
		EWHerschel400Objects                 = 37,
		EWAlgolTypeVariableStars             = 38, // http://www.sai.msu.su/gcvs/gcvs/vartype.htm
		EWClassicalCepheidsTypeVariableStars = 39, // http://www.sai.msu.su/gcvs/gcvs/vartype.htm
		EWCarbonStars                        = 40,
		EWBariumStars                        = 41,
		EWNone	// stop gapper for syntax reasons
	};
};

//! Derived from QTreeWidgetItem class with customized sort
class ACCelPosTreeWidgetItem : public QTreeWidgetItem
{
public:
	ACCelPosTreeWidgetItem(QTreeWidget* parent)
		: QTreeWidgetItem(parent)
	{
	}

private:
	bool operator < (const QTreeWidgetItem &other) const override
	{
		int column = treeWidget()->sortColumn();

		if (column == AstroCalcDialog::CColumnName)
		{
			static const QRegularExpression dso("^(\\w+)\\s*(\\d+)\\s*(.*)$");
			static const QRegularExpression mp("^[(](\\d+)[)]\\s(.+)$");
			QRegularExpressionMatch dsoMatch=dso.match(text(column));
			QRegularExpressionMatch mpMatch=mp.match(text(column));
			QRegularExpressionMatch dsoOtherMatch=dso.match(other.text(column));
			QRegularExpressionMatch mpOtherMatch=mp.match(other.text(column));
			int a = 0, b = 0;
			if (dsoMatch.hasMatch())
				a = dsoMatch.captured(2).toInt();
			if (a==0 && mpMatch.hasMatch())
				a = mpMatch.captured(1).toInt();
			if (dsoOtherMatch.hasMatch())
				b = dsoOtherMatch.captured(2).toInt();
			if (b==0 && mpOtherMatch.hasMatch())
				b = mpOtherMatch.captured(1).toInt();
			if (a>0 && b>0)
				return a < b;
			else
				return text(column).toLower() < other.text(column).toLower();
		}
		else if (column == AstroCalcDialog::CColumnRA || column == AstroCalcDialog::CColumnDec || column == AstroCalcDialog::CColumnMaxElevation || column == AstroCalcDialog::CColumnElongation)
		{
			return StelUtils::getDecAngle(text(column)) < StelUtils::getDecAngle(other.text(column));
		}
		else if (column == AstroCalcDialog::CColumnMagnitude || column == AstroCalcDialog::CColumnAngularSize || column == AstroCalcDialog::CColumnExtra)
		{
			return text(column).toFloat() < other.text(column).toFloat();
		}
		else if (column == AstroCalcDialog::CColumnTransit)
		{
			return StelUtils::hmsStrToHours(text(column).append("00s")) < StelUtils::hmsStrToHours(other.text(column).append("00s"));
		}
		else
		{
			return text(column).toLower() < other.text(column).toLower();
		}
	}
};

//! Derived from QTreeWidgetItem class with customized sort
class AHECPosTreeWidgetItem : public QTreeWidgetItem
{
public:
	AHECPosTreeWidgetItem(QTreeWidget* parent)
		: QTreeWidgetItem(parent)
	{
	}

private:
	bool operator < (const QTreeWidgetItem &other) const override
	{
		int column = treeWidget()->sortColumn();

		if (column == AstroCalcDialog::HECColumnLatitude || column == AstroCalcDialog::HECColumnLongitude)
		{
			return StelUtils::getDecAngle(text(column)) < StelUtils::getDecAngle(other.text(column));
		}
		else if (column == AstroCalcDialog::HECColumnDistance)
		{
			return data(column, Qt::UserRole).toFloat() < other.data(column, Qt::UserRole).toFloat();
		}
		else
		{
			return text(column).toLower() < other.text(column).toLower();
		}
	}
};

//! Derived from QTreeWidgetItem class with customized sort
class ACEphemTreeWidgetItem : public QTreeWidgetItem
{
public:
	ACEphemTreeWidgetItem(QTreeWidget* parent)
		: QTreeWidgetItem(parent)
	{
	}

private:
	bool operator < (const QTreeWidgetItem &other) const override
	{
		int column = treeWidget()->sortColumn();

		if (column == AstroCalcDialog::EphemerisDate)
		{
			return data(column, Qt::UserRole).toFloat() < other.data(column, Qt::UserRole).toFloat();
		}
		else if (column == AstroCalcDialog::EphemerisRA || column == AstroCalcDialog::EphemerisDec)
		{
			return StelUtils::getDecAngle(text(column)) < StelUtils::getDecAngle(other.text(column));
		}
		else if (column == AstroCalcDialog::EphemerisMagnitude || column == AstroCalcDialog::EphemerisDistance)
		{
			return text(column).toFloat() < other.text(column).toFloat();
		}
		else
		{
			return text(column).toLower() < other.text(column).toLower();
		}
	}
};

//! Derived from QTreeWidgetItem class with customized sort
class ACRTSTreeWidgetItem : public QTreeWidgetItem
{
public:
	ACRTSTreeWidgetItem(QTreeWidget* parent)
		: QTreeWidgetItem(parent)
	{
	}

private:
	bool operator < (const QTreeWidgetItem &other) const override
	{
		int column = treeWidget()->sortColumn();

		if (column == AstroCalcDialog::RTSRiseDate || column == AstroCalcDialog::RTSTransitDate || column == AstroCalcDialog::RTSSetDate)
		{
			return data(column, Qt::UserRole).toFloat() < other.data(column, Qt::UserRole).toFloat();
		}
		else if (column == AstroCalcDialog::RTSMagnitude)
		{
			return text(column).toFloat() < other.text(column).toFloat();
		}
		else if (column == AstroCalcDialog::RTSTransitAltitude || column == AstroCalcDialog::RTSElongation || column == AstroCalcDialog::RTSAngularDistance)
		{
			return StelUtils::getDecAngle(text(column)) < StelUtils::getDecAngle(other.text(column));
		}
		else
		{
			return text(column).toLower() < other.text(column).toLower();
		}
	}
};

//! Derived from QTreeWidgetItem class with customized sort
class ACLunarEclipseTreeWidgetItem : public QTreeWidgetItem
{
public:
	ACLunarEclipseTreeWidgetItem(QTreeWidget* parent)
		: QTreeWidgetItem(parent)
	{
	}

private:
	bool operator < (const QTreeWidgetItem &other) const override
	{
		int column = treeWidget()->sortColumn();

		if (column == AstroCalcDialog::LunarEclipseDate || column == AstroCalcDialog::LunarEclipsePMag || column == AstroCalcDialog::LunarEclipseVisConditions )
		{
			return data(column, Qt::UserRole).toFloat() < other.data(column, Qt::UserRole).toFloat();
		}
		else if (column == AstroCalcDialog::LunarEclipseGamma)
		{
			return text(column).toFloat() < other.text(column).toFloat();
		}
		else
		{
			return text(column).toLower() < other.text(column).toLower();
		}
	}
};

//! Besselian elements for lunar eclipse
class LunarEclipseBessel
{
public:
	LunarEclipseBessel(double &besX, double &besY, double &besL1, double &besL2, double &besL3, double &latDeg, double &lngDeg);
};

//! Iteration to compute contact times of lunar eclipse
class LunarEclipseIteration
{
public:
	LunarEclipseIteration(double &JD, double &positionAngle, double &axisDistance, bool beforeMaximum, int eclipseType);
};

//! Derived from QTreeWidgetItem class, but currently nothing else.
class ACLunarEclipseContactsTreeWidgetItem : public QTreeWidgetItem
{
public:
	ACLunarEclipseContactsTreeWidgetItem(QTreeWidget* parent)
		: QTreeWidgetItem(parent)
	{
	}
};

//! Derived from QTreeWidgetItem class with customized sort
class ACSolarEclipseTreeWidgetItem : public QTreeWidgetItem
{
public:
	ACSolarEclipseTreeWidgetItem(QTreeWidget* parent)
		: QTreeWidgetItem(parent)
	{
	}

private:
	bool operator < (const QTreeWidgetItem &other) const override
	{
		int column = treeWidget()->sortColumn();

		if (column == AstroCalcDialog::SolarEclipseDate || column == AstroCalcDialog::SolarEclipseLatitude || column == AstroCalcDialog::SolarEclipseLongitude || column == AstroCalcDialog::SolarEclipsePathwidth || column == AstroCalcDialog::SolarEclipseAltitude || column == AstroCalcDialog::SolarEclipseDuration)
		{
			return data(column, Qt::UserRole).toFloat() < other.data(column, Qt::UserRole).toFloat();
		}
		else if (column == AstroCalcDialog::SolarEclipseGamma)
		{
			return text(column).toFloat() < other.text(column).toFloat();
		}
		else
		{
			return text(column).toLower() < other.text(column).toLower();
		}
	}
};

//! Derived from QTreeWidgetItem class with customized sort
class ACSolarEclipseContactsTreeWidgetItem : public QTreeWidgetItem
{
public:
	ACSolarEclipseContactsTreeWidgetItem(QTreeWidget* parent)
		: QTreeWidgetItem(parent)
	{
	}

private:
	bool operator < (const QTreeWidgetItem &other) const override
	{
		int column = treeWidget()->sortColumn();

		if (column == AstroCalcDialog::SolarEclipseContactDate)
		{
			return data(column, Qt::UserRole).toFloat() < other.data(column, Qt::UserRole).toFloat();
		}
		else
		{
			return text(column).toLower() < other.text(column).toLower();
		}
	}
};

// Class to compute parameters from Besselian elements
class BesselParameters
{
public:
	BesselParameters(double &xdot, double &ydot, double &ddot, double &mudot,
	double &ldot, double &etadot, double &bdot, double &cdot, bool penumbra);
};

//! Derived from QTreeWidgetItem class with customized sort
class ACSolarEclipseLocalTreeWidgetItem : public QTreeWidgetItem
{
public:
	ACSolarEclipseLocalTreeWidgetItem(QTreeWidget* parent)
		: QTreeWidgetItem(parent)
	{
	}

private:
	bool operator < (const QTreeWidgetItem &other) const override
	{
		int column = treeWidget()->sortColumn();

		if (column == AstroCalcDialog::SolarEclipseLocalDate)
		{
			return data(column, Qt::UserRole).toFloat() < other.data(column, Qt::UserRole).toFloat();
		}		
		else
		{
			return text(column).toLower() < other.text(column).toLower();
		}
	}
};

//! Derived from QTreeWidgetItem class with customized sort
class ACTransitTreeWidgetItem : public QTreeWidgetItem
{
public:
	ACTransitTreeWidgetItem(QTreeWidget* parent)
		: QTreeWidgetItem(parent)
	{
	}

private:
	bool operator < (const QTreeWidgetItem &other) const
	{
		int column = treeWidget()->sortColumn();

		if (column == AstroCalcDialog::TransitDate || column == AstroCalcDialog::TransitContact1 || column == AstroCalcDialog::TransitContact2 || column == AstroCalcDialog::TransitContact3 || column == AstroCalcDialog::TransitContact4 || column == AstroCalcDialog::TransitMid || column == AstroCalcDialog::TransitSeparation || column == AstroCalcDialog::TransitDuration || column == AstroCalcDialog::TransitObservableDuration)
		{
			return data(column, Qt::UserRole).toFloat() < other.data(column, Qt::UserRole).toFloat();
		}		
		else
		{
			return text(column).toLower() < other.text(column).toLower();
		}
	}
};

//! Besselian elements for transit of Mercury and Venus across the Sun
class TransitBessel
{
public:
	TransitBessel(PlanetP object, double &besX, double &besY,
	double &besDec, double &besTf1, double &besTf2, double &besL1, double &besL2, double &besMu);
};

//! Derived from QTreeWidgetItem class with customized sort
class ACPhenTreeWidgetItem : public QTreeWidgetItem
{
public:
	ACPhenTreeWidgetItem(QTreeWidget* parent)
		: QTreeWidgetItem(parent)
	{
	}

private:
	bool operator < (const QTreeWidgetItem &other) const override
	{
		int column = treeWidget()->sortColumn();

		if (column == AstroCalcDialog::PhenomenaSeparation || column == AstroCalcDialog::PhenomenaElevation || column == AstroCalcDialog::PhenomenaElongation || column == AstroCalcDialog::PhenomenaAngularDistance)
		{
			return StelUtils::getDecAngle(text(column)) < StelUtils::getDecAngle(other.text(column));
		}
		else if (column == AstroCalcDialog::PhenomenaDate)
		{
			return data(column, Qt::UserRole).toFloat() < other.data(column, Qt::UserRole).toFloat();
		}
		else if (column == AstroCalcDialog::PhenomenaMagnitude1 || column == AstroCalcDialog::PhenomenaMagnitude2)
		{
			return text(column).toFloat() < other.text(column).toFloat();
		}
		else
		{
			return text(column).toLower() < other.text(column).toLower();
		}
	}
};

//! Derived from QTreeWidgetItem class with customized sort
class WUTTreeWidgetItem : public QTreeWidgetItem
{
public:
	WUTTreeWidgetItem(QTreeWidget* parent)
		: QTreeWidgetItem(parent)
	{
	}

private:
	bool operator < (const QTreeWidgetItem &other) const override
	{
		int column = treeWidget()->sortColumn();

		if (column == AstroCalcDialog::WUTObjectName)
		{
			static const QRegularExpression dso("^(\\w+)\\s*(\\d+)\\s*(.*)$");
			static const QRegularExpression mp("^[(](\\d+)[)]\\s(.+)$");
			QRegularExpressionMatch dsoMatch=dso.match(text(column));
			QRegularExpressionMatch mpMatch=mp.match(text(column));
			QRegularExpressionMatch dsoOtherMatch=dso.match(other.text(column));
			QRegularExpressionMatch mpOtherMatch=mp.match(other.text(column));
			int a = 0, b = 0;
			if (dsoMatch.hasMatch())
				a = dsoMatch.captured(2).toInt();
			if (a==0 && mpMatch.hasMatch())
				a = mpMatch.captured(1).toInt();
			if (dsoOtherMatch.hasMatch())
				b = dsoOtherMatch.captured(2).toInt();
			if (b==0 && mpOtherMatch.hasMatch())
				b = mpOtherMatch.captured(1).toInt();
			if (a>0 && b>0)
				return a < b;
			else
				return text(column).toLower() < other.text(column).toLower();
		}
		else if (column == AstroCalcDialog::WUTMagnitude)
		{
			return text(column).toFloat() < other.text(column).toFloat();
		}
		else if (column == AstroCalcDialog::WUTRiseTime || column == AstroCalcDialog::WUTTransitTime || column == AstroCalcDialog::WUTSetTime)
		{
			return StelUtils::hmsStrToHours(text(column).append("00s")) < StelUtils::hmsStrToHours(other.text(column).append("00s"));
		}
		else if (column == AstroCalcDialog::WUTMaxElevation || column == AstroCalcDialog::WUTAngularSize)
		{
			return StelUtils::getDecAngle(text(column)) < StelUtils::getDecAngle(other.text(column));
		}
		else
		{
			return text(column).toLower() < other.text(column).toLower();
		}
	}
};

#endif // ASTROCALCDIALOG_HPP
