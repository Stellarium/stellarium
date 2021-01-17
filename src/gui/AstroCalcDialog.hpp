/*
 * Stellarium
 * 
 * Copyright (C) 2015 Alexander Wolf
 * Copyright (C) 2016 Nick Fedoseev (visualization of ephemeris)
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

#include "StelDialog.hpp"
#include "StelCore.hpp"
#include "Planet.hpp"
#include "SolarSystem.hpp"
#include "Nebula.hpp"
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
};
Q_DECLARE_METATYPE(Ephemeris)

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

	//! Defines the number and the order of the columns in the transit table
	//! @enum TransitColumns
	enum TransitColumns {
		TransitCOName,          //! name of celestial object
		TransitDate,            //! date and time of transit
		TransitAltitude,        //! altitude
		TransitMagnitude,       //! magnitude
		TransitElongation,      //! elongation (from the Sun)
		TransitAngularDistance, //! angular distance (from the Moon)
		TransitCount            //! total number of columns
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
		Conjuction		= 0,
		Opposition		= 1,
		GreatestElongation	= 2,
		StationaryPoint		= 3,
		OrbitalPoint		= 4
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
		WUTCount                //! total number of columns
	};

	//! Defines the type of graphs
	//! @enum GraphsTypes
	enum GraphsTypes {
		GraphMagnitudeVsTime        =  1,
		GraphPhaseVsTime            =  2,
		GraphDistanceVsTime         =  3,
		GraphElongationVsTime       =  4,
		GraphAngularSizeVsTime      =  5,
		GraphPhaseAngleVsTime       =  6,
		GraphHDistanceVsTime        =  7,
		GraphTransitAltitudeVsTime  =  8,
		GraphRightAscensionVsTime   =  9,
		GraphDeclinationVsTime      = 10
	};

	AstroCalcDialog(QObject* parent);
	virtual ~AstroCalcDialog() Q_DECL_OVERRIDE;

	static QVector<Ephemeris> EphemerisList;
	static int DisplayedPositionIndex;


public slots:
	void retranslate() Q_DECL_OVERRIDE;
		
signals:
	//! This signal is emitted when the graph day changed.
	void graphDayChanged();

protected:
        //! Initialize the dialog widgets and connect the signals/slots.
	virtual void createDialogContent() Q_DECL_OVERRIDE;
        Ui_astroCalcDialogForm *ui;

private slots:
	void currentCelestialPositions();
	void populateCelestialCategoryList();
	void saveCelestialPositions();
	void selectCurrentCelestialPosition(const QModelIndex &modelIndex);

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

	//! Calculating the transits for selected celestial body and fill the list.
	void generateTransits();
	void cleanupTransits();
	void selectCurrentTransit(const QModelIndex &modelIndex);
	void saveTransits();
	void setTransitCelestialBodyName();

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

	//! Compute planetary data
	void saveFirstCelestialBody(int index);
	void saveSecondCelestialBody(int index);
	void computePlanetaryData();
	void drawDistanceGraph();
	void mouseOverDistanceGraph(QMouseEvent *event);

	void drawAngularDistanceGraph();
	void drawAngularDistanceLimitLine();
	void saveAngularDistanceLimit(int limit);

	//! Draw diagram 'Altitude vs. Time'
	void drawAltVsTimeDiagram();
	//! Draw vertical line 'Now' on diagram 'Altitude vs. Time'
	void drawCurrentTimeDiagram();
	//! Draw vertical line of meridian passage time on diagram 'Altitude vs. Time'
	void drawTransitTimeDiagram();	
	//! Show info from graphs under mouse cursor
	void mouseOverLine(QMouseEvent *event);
	void saveAltVsTimeSunFlag(bool state);
	void saveAltVsTimeMoonFlag(bool state);
	void saveAltVsTimePositiveFlag(bool state);
	void saveAltVsTimePositiveLimit(int limit);

	//! Draw diagram 'Azimuth vs. Time'
	void drawAziVsTimeDiagram();
	//! Show info from graphs under mouse cursor
	void mouseOverAziLine(QMouseEvent *event);

	//! Set time by clicking inside graph areas
	void altTimeClick(QMouseEvent* event);
	void aziTimeClick(QMouseEvent* event);

	//! handle events that are otherwise "lost" when dialog not visible
	void handleVisibleEnabled();

	void saveGraphsCelestialBody(int index);
	void saveGraphsFirstId(int index);
	void saveGraphsSecondId(int index);
	void updateGraphsDuration(int duration);
	void drawXVsTimeGraphs();

	void drawMonthlyElevationGraph();
	void updateMonthlyElevationTime();
	void syncMonthlyElevationTime();
	void saveMonthlyElevationPositiveFlag(bool state);
	void saveMonthlyElevationPositiveLimit(int limit);

	// WUT
	void saveWutMagnitudeLimit(double mag);
	void saveWutMinAngularSizeLimit();
	void saveWutMaxAngularSizeLimit();
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

	void updateSolarSystemData();
	void populateCelestialNames(QString);
	void showExtraEphemerisDialog();
	void showCustomStepsDialog();

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
	QSettings* conf;
	QTimer *currentTimeLine;
	QHash<QString,int> wutCategories;

	void saveTableAsCSV(const QString& fileName, QTreeWidget* tWidget, QStringList& headers);
	void saveTableAsBookmarks(const QString& fileName, QTreeWidget* tWidget);

	//! Update header names for celestial positions tables
	void setCelestialPositionsHeaderNames();
	//! Update header names for ephemeris table
	void setEphemerisHeaderNames();
	//! update header names for transit table
	void setTransitHeaderNames();
	//! Update header names for phenomena table
	void setPhenomenaHeaderNames();
	//! Update header names for WUT table
	void setWUTHeaderNames(const bool magnitude = true, const bool separation = false);

	//! Init header and list of celestial positions
	void initListCelestialPositions();
	//! Init header and list of ephemeris
	void initListEphemeris();
	//! Init header and list of transits
	void initListTransit();
	//! Init header and list of phenomena
	void initListPhenomena();
	//! Init header and list of WUT
	void initListWUT(const bool magnitude = true, const bool separation = false);

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
	void prepareAziVsTimeAxesAndGraph();
	void prepareXVsTimeAxesAndGraph();
	void prepareMonthlyEleveationAxesAndGraph();
	void prepareDistanceAxesAndGraph();
	void prepareAngularDistanceAxesAndGraph();
	//! Populates the drop-down list of time intervals for WUT tool.
	void populateTimeIntervalsList();	
	double computeGraphValue(const PlanetP &ssObj, const int graphType);

	void populateFunctionsList();
	double computeMaxElevation(StelObjectP obj);

	void adjustCelestialPositionsColumns();
	void adjustWUTColumns();
	void adjustPhenomenaColumns();

	void enableCustomEphemerisTimeStepButton();
	double getCustomTimeStep();
	void reGenerateEphemeris(bool withSelection);

	//! Format RA/Dec or Az/Alt coordinates into nice strings.
	//! @arg horizontal coord are horizontal (alt-azimuthal). Use degrees/degrees. Else use Hours/degrees.
	//! @arg southAzimuth (relevant only for horizontal=true) count azimuth from south.
	//! @arg decimalDegrees use decimal format, not DMS/HMS
	//! @return QPair(lngStr, latStr) formatted output strings
	static QPair<QString, QString> getStringCoordinates(const Vec3d coord, const bool horizontal, const bool southAzimuth, const bool decimalDegrees);
	void fillWUTTable(QString objectName, QString designation, float magnitude, Vec3f RTSTime, double maxElevation, double angularSize, bool decimalDegrees = false);
	void fillCelestialPositionTable(QString objectName, QString RA, QString Dec, float magnitude,
					QString angularSize, QString angularSizeToolTip, QString extraData,
					QString extraDataToolTip, QString transitTime, QString maxElevation,
					QString sElongation, QString objectType);

	//! Calculation conjunctions and oppositions.
	//! @note Ported from KStars, should be improved, because this feature calculates
	//! angular separation ("conjunction" defined as equality of right ascension
	//! of two bodies), and current solution is not accurate and slow.
	//! @note modes: 0 - conjuction, 1 - opposition, 2 - greatest elongation
	QMap<double, double> findClosestApproach(PlanetP& object1, StelObjectP& object2, double startJD, double stopJD, double maxSeparation, int mode);
	// TODO: Doc?
	double findDistance(double JD, PlanetP object1, StelObjectP object2, int mode);
	// TODO: Doc?
	double findInitialStep(double startJD, double stopJD, QStringList objects);
	// TODO: Doc?
	bool findPrecise(QPair<double, double>* out, PlanetP object1, StelObjectP object2, double JD, double step, int prevSign, int mode);
	// TODO: Doc?
	void fillPhenomenaTable(const QMap<double, double> list, const PlanetP object1, const StelObjectP object2, int mode);
	// TODO: Doc?
	void fillPhenomenaTable(const QMap<double, double> list, const PlanetP object1, const NebulaP object2);
	//! @note modes: 0 - conjuction, 1 - opposition, 2 - greatest elongation
	void fillPhenomenaTable(const QMap<double, double> list, const PlanetP object1, const PlanetP object2, int mode);
	void fillPhenomenaTableVis(QString phenomenType, double JD, QString firstObjectName, float firstObjectMagnitude,
				   QString secondObjectName, float secondObjectMagnitude, QString separation, QString elevation,
				   QString elongation, QString angularDistance, QString elongTooltip="", QString angDistTooltip="");
	//! Calculation greatest elongations
	QMap<double, double> findGreatestElongationApproach(PlanetP& object1, StelObjectP& object2, double startJD, double stopJD);
	bool findPreciseGreatestElongation(QPair<double, double>* out, PlanetP object1, StelObjectP object2, double JD, double stopJD, double step);
	//! Calculation stationary points
	QMap<double, double> findStationaryPointApproach(PlanetP& object1, double startJD, double stopJD);
	bool findPreciseStationaryPoint(QPair<double, double>* out, PlanetP object, double JD, double stopJD, double step, bool retrograde);
	double findRightAscension(double JD, PlanetP object);
	//! Calculation perihelion and aphelion points
	QMap<double, double> findOrbitalPointApproach(PlanetP& object1, double startJD, double stopJD);
	bool findPreciseOrbitalPoint(QPair<double, double>* out, PlanetP object1, double JD, double stopJD, double step, bool minimal);
	double findHeliocentricDistance(double JD, PlanetP object1);

	bool plotAltVsTime, plotAltVsTimeSun, plotAltVsTimeMoon, plotAltVsTimePositive, plotMonthlyElevation, plotMonthlyElevationPositive, plotDistanceGraph, plotAngularDistanceGraph, plotAziVsTime;
	int altVsTimePositiveLimit, monthlyElevationPositiveLimit, graphsDuration;
	QStringList ephemerisHeader, phenomenaHeader, positionsHeader, wutHeader, transitHeader;
	static float brightLimit;
	static double minY, maxY, minYme, maxYme, minYsun, maxYsun, minYmoon, maxYmoon, transitX, minY1, maxY1, minY2, maxY2,
			     minYld, maxYld, minYad, maxYad, minYadm, maxYadm, minYaz, maxYaz;
	static QString yAxis1Legend, yAxis2Legend;
	static const QString dash, delimiter;

	//! Make sure that no tabs icons are outside of the viewport.
	//! @todo Limit the width to the width of the screen *available to the window*.
	void updateTabBarListWidgetWidth();

	void enableAngularLimits(bool enable);

	//! Set clicked time in AstroCalc AltVSTime/AziVsTime graphs
	void setClickedTime(double posx);

	//! Memorize day for detecting rollover to next/prev one
	int oldGraphJD;

	//! Remember to redraw active plot when dialog becomes visible
	bool graphPlotNeedsRefresh;

	enum WUTCategory {
		EWPlanets							= 0,
		EWBrightStars						= 1,
		EWBrightNebulae					= 2,
		EWDarkNebulae					= 3,
		EWGalaxies						= 4,
		EWOpenStarClusters					= 5,
		EWAsteroids						= 6,
		EWComets						= 7,
		EWPlutinos						= 8,
		EWDwarfPlanets					= 9,
		EWCubewanos						= 10,
		EWScatteredDiscObjects				= 11,
		EWOortCloudObjects					= 12,
		EWSednoids						= 13,
		EWPlanetaryNebulae					= 14,
		EWBrightDoubleStars				= 15,
		EWBrightVariableStars				= 16,
		EWBrightStarsWithHighProperMotion		= 17,
		EWSymbioticStars					= 18,
		EWEmissionLineStars				= 19,
		EWSupernovaeCandidates			= 20,
		EWSupernovaeRemnantCandidates		= 21,
		EWSupernovaeRemnants				= 22,
		EWClustersOfGalaxies				= 23,
		EWInterstellarObjects				= 24,
		EWGlobularStarClusters				= 25,
		EWRegionsOfTheSky					= 26,
		EWActiveGalaxies					= 27,
		EWPulsars							= 28,
		EWExoplanetarySystems				= 29,
		EWBrightNovaStars					= 30,
		EWBrightSupernovaStars				= 31,
		EWInteractingGalaxies				= 32,
		EWDeepSkyObjects					= 33,
		EWMessierObjects					= 34,
		EWNGCICObjects					= 35,
		EWCaldwellObjects					= 36,
		EWHerschel400Objects				= 37,
		EWAlgolTypeVariableStars				= 38,	// http://www.sai.msu.su/gcvs/gcvs/vartype.htm
		EWClassicalCepheidsTypeVariableStars	= 39,	// http://www.sai.msu.su/gcvs/gcvs/vartype.htm
		EWNone	// stop gapper for syntax reasons
	};
};

// Reimplements the QTreeWidgetItem class to fix the sorting bug
class ACCelPosTreeWidgetItem : public QTreeWidgetItem
{
public:
	ACCelPosTreeWidgetItem(QTreeWidget* parent)
		: QTreeWidgetItem(parent)
	{
	}

private:
	bool operator < (const QTreeWidgetItem &other) const
	{
		int column = treeWidget()->sortColumn();

		if (column == AstroCalcDialog::CColumnName)
		{
			QRegExp dso("^(\\w+)\\s*(\\d+)\\s*(.*)$");
			QRegExp mp("^[(](\\d+)[)]\\s(.+)$");
			int a = 0, b = 0;			
			if (dso.exactMatch(text(column)))
				a = dso.cap(2).toInt();
			if (a==0 && mp.exactMatch(text(column)))
				a = mp.cap(1).toInt();
			if (dso.exactMatch(other.text(column)))
				b = dso.cap(2).toInt();
			if (b==0 && mp.exactMatch(other.text(column)))
				b = mp.cap(1).toInt();
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

// Reimplements the QTreeWidgetItem class to fix the sorting bug
class ACEphemTreeWidgetItem : public QTreeWidgetItem
{
public:
	ACEphemTreeWidgetItem(QTreeWidget* parent)
		: QTreeWidgetItem(parent)
	{
	}

private:
	bool operator < (const QTreeWidgetItem &other) const
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

// Reimplements the QTreeWidgetItem class to fix the sorting bug
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

		if (column == AstroCalcDialog::TransitDate)
		{
			return data(column, Qt::UserRole).toFloat() < other.data(column, Qt::UserRole).toFloat();
		}
		else if (column == AstroCalcDialog::TransitMagnitude)
		{
			return text(column).toFloat() < other.text(column).toFloat();
		}
		else if (column == AstroCalcDialog::TransitAltitude || column == AstroCalcDialog::TransitElongation || column == AstroCalcDialog::TransitAngularDistance)
		{
			return StelUtils::getDecAngle(text(column)) < StelUtils::getDecAngle(other.text(column));
		}
		else
		{
			return text(column).toLower() < other.text(column).toLower();
		}
	}
};

// Reimplements the QTreeWidgetItem class to fix the sorting bug
class ACPhenTreeWidgetItem : public QTreeWidgetItem
{
public:
	ACPhenTreeWidgetItem(QTreeWidget* parent)
		: QTreeWidgetItem(parent)
	{
	}

private:
	bool operator < (const QTreeWidgetItem &other) const
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

// Reimplements the QTreeWidgetItem class to fix the sorting bug
class WUTTreeWidgetItem : public QTreeWidgetItem
{
public:
	WUTTreeWidgetItem(QTreeWidget* parent)
		: QTreeWidgetItem(parent)
	{
	}

private:
	bool operator < (const QTreeWidgetItem &other) const
	{
		int column = treeWidget()->sortColumn();

		if (column == AstroCalcDialog::WUTObjectName)
		{
			QRegExp dso("^(\\w+)\\s*(\\d+)\\s*(.*)$");
			QRegExp mp("^[(](\\d+)[)]\\s(.+)$");
			int a = 0, b = 0;
			if (dso.exactMatch(text(column)))
				a = dso.cap(2).toInt();
			if (a==0 && mp.exactMatch(text(column)))
				a = mp.cap(1).toInt();
			if (dso.exactMatch(other.text(column)))
				b = dso.cap(2).toInt();
			if (b==0 && mp.exactMatch(other.text(column)))
				b = mp.cap(1).toInt();
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
