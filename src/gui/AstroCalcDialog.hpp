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
#include "StarMgr.hpp"
#include "StelUtils.hpp"

class Ui_astroCalcDialogForm;
class QListWidgetItem;
class QSortFilterProxyModel;
class QStringListModel;
class AstroCalcExtraEphemerisDialog;

struct Ephemeris
{
	Vec3d coord;
	int colorIndex;
	QString objDate;
	float magnitude;
};
Q_DECLARE_METATYPE(Ephemeris)

class AstroCalcDialog : public StelDialog
{
	Q_OBJECT

public:
	//! Defines the number and the order of the columns in the table that lists celestial bodies positions
	//! @enum CPositionsColumns
	enum CPositionsColumns {
		CColumnName,			//! name of object
		CColumnRA,			//! right ascension
		CColumnDec,			//! declination
		CColumnMagnitude,		//! magnitude
		CColumnAngularSize,	//! angular size
		CColumnExtra,			//! extra data (surface brightness, separation, period, etc.)
		CColumnTransit,		//! time of transit
		CColumnType,			//! type of object
		CColumnCount			//! total number of columns
	};

	//! Defines the number and the order of the columns in the ephemeris table
	//! @enum EphemerisColumns
	enum EphemerisColumns {
		EphemerisCOName,		//! name of celestial object
		EphemerisDate,		//! date and time of ephemeris		
		EphemerisRA,			//! right ascension
		EphemerisDec,			//! declination
		EphemerisMagnitude,	//! magnitude
		EphemerisPhase,		//! phase
		EphemerisDistance,		//! distance
		EphemerisElongation,	//! elongation
		EphemerisCount		//! total number of columns
	};

	//! Defines the number and the order of the columns in the phenomena table
	//! @enum PhenomenaColumns
	enum PhenomenaColumns {
		PhenomenaType,			//! type of phenomena
		PhenomenaDate,			//! date and time of ephemeris
		PhenomenaObject1,			//! first object
		PhenomenaMagnitude1,		//! magnitude of first object
		PhenomenaObject2,			//! second object
		PhenomenaMagnitude2,		//! magnitude of second object
		PhenomenaSeparation,		//! angular separation
		PhenomenaElongation,		//! elongation (from the Sun)
		PhenomenaAngularDistance,	//! angular distance (from the Moon)
		PhenomenaCount			//! total number of columns
	};

	enum PhenomenaTypeIndex {
		Conjuction		= 0,
		Opposition		= 1,
		GreatestElongation	= 2,
		StationaryPoint		= 3
	};

	//! Defines the number and the order of the columns in the WUT tool
	//! @enum WUTColumns
	enum WUTColumns {
		WUTObjectName,		//! object name
		WUTMagnitude,		//! magnitude
		WUTRiseTime,			//! rise time
		WUTTransitTime,		//! transit time
		WUTSetTime,			//! set time
		WUTAngularSize,		//! angular size
		WUTCount			//! total number of columns
	};

	//! Defines the type of graphs
	//! @enum GraphsTypes
	enum GraphsTypes {
		GraphMagnitudeVsTime		= 1,
		GraphPhaseVsTime			= 2,
		GraphDistanceVsTime		= 3,
		GraphElongationVsTime		= 4,
		GraphAngularSizeVsTime		= 5,
		GraphPhaseAngleVsTime		= 6,
		GraphHDistanceVsTime		= 7,
		GraphTransitAltitudeVsTime	= 8,
		GraphRightAscensionVsTime	= 9,
		GraphDeclinationVsTime		= 10
	};

	AstroCalcDialog(QObject* parent);
	virtual ~AstroCalcDialog();

	//! Notify that the application style changed
	void styleChanged();

	static QVector<Ephemeris> EphemerisList;
	static int DisplayedPositionIndex;


public slots:
        void retranslate();
		
signals:
	//! This signal is emitted when the graph day changed.
	void graphDayChanged();

protected:
        //! Initialize the dialog widgets and connect the signals/slots.
        virtual void createDialogContent();
        Ui_astroCalcDialogForm *ui;

private slots:
	void currentCelestialPositions();
	void populateCelestialCategoryList();
	void saveCelestialPositions();
	void selectCurrentCelestialPosition(const QModelIndex &modelIndex);

	void saveCelestialPositionsMagnitudeLimit(double mag);
	void saveCelestialPositionsHorizontalCoordinatesFlag(bool b);
	void saveCelestialPositionsCategory(int index);

	//! Calculate ephemeris for selected celestial body and fill the list.
	void generateEphemeris();
	void cleanupEphemeris();
	void selectCurrentEphemeride(const QModelIndex &modelIndex);
	void saveEphemeris();
	void onChangedEphemerisPosition(const QModelIndex &modelIndex);	
	void reGenerateEphemeris();

	void saveEphemerisCelestialBody(int index);
	void saveEphemerisTimeStep(int index);
	void initEphemerisFlagNakedEyePlanets(void);
	void saveEphemerisFlagNakedEyePlanets(bool flag);

	//! Calculate phenomena for selected celestial body and fill the list.
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

	void updateAstroCalcData();

	void changePage(QListWidgetItem *current, QListWidgetItem *previous);
	void changePCTab(int index);
	void changeGraphsTab(int index);

	void updateSolarSystemData();

	void showExtraEphemerisDialog();

private:
	class AstroCalcExtraEphemerisDialog* extraEphemerisDialog;
	class StelCore* core;
	class SolarSystem* solarSystem;
	class NebulaMgr* dsoMgr;
	class StarMgr* starMgr;
	class StelObjectMgr* objectMgr;
	class StelLocaleMgr* localeMgr;
	class StelMovementMgr* mvMgr;
	QStringListModel* wutModel;
	QSortFilterProxyModel *proxyModel;
	QSettings* conf;
	QTimer *currentTimeLine;
	QHash<QString,int> wutCategories;

	void saveTableAsCSV(const QString& fileName, QTreeWidget* tWidget, QStringList& headers);

	//! Update header names for celestial positions tables
	void setCelestialPositionsHeaderNames();
	//! Update header names for ephemeris table
	void setEphemerisHeaderNames();
	//! Update header names for phenomena table
	void setPhenomenaHeaderNames();
	//! Update header names for WUT table
	void setWUTHeaderNames(const bool magnitude = true, const bool separation = false);

	//! Init header and list of celestial positions
	void initListCelestialPositions();
	//! Init header and list of ephemeris
	void initListEphemeris();
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
	//! Populates the drop-down list of major planets.
	void populateMajorPlanetList();
	//! Prepare graph settings
	void prepareAxesAndGraph();
	void prepareAziVsTimeAxesAndGraph();
	void prepareXVsTimeAxesAndGraph();
	void prepareMonthlyEleveationAxesAndGraph();
	void prepareDistanceAxesAndGraph();
	void prepareAngularDistanceAxesAndGraph();
	//! Populates the drop-down list of time intervals for WUT tool.
	void populateTimeIntervalsList();
	//! Populates the list of groups for WUT tool.
	void populateWutGroups();	
	double computeGraphValue(const PlanetP &ssObj, const int graphType);

	void populateFunctionsList();

	void adjustWUTColumns();

	QPair<QString, QString> getStringCoordinates(const Vec3d coord, const bool horizon, const bool southAzimuth, const bool decimalDegrees);
	void fillWUTTable(QString objectName, QString designation, double magnitude, Vec3f RTSTime, double angularSize, bool decimalDegrees = false);
	void fillCelestialPositionTable(QString objectName, QString RA, QString Dec, double magnitude,
					QString angularSize, QString angularSizeToolTip, QString extraData,
					QString extraDataToolTip, QString transitTime, QString objectType);

	//! Calculation conjunctions and oppositions.
	//! @note Ported from KStars, should be improved, because this feature calculate
	//! angular separation ("conjunction" defined as equality of right ascension
	//! of two body) and current solution is not accurate and slow.	
	//! @note modes: 0 - conjuction, 1 - opposition, 2 - greatest elongation
	QMap<double, double> findClosestApproach(PlanetP& object1, StelObjectP& object2, double startJD, double stopJD, double maxSeparation, int mode);
	double findDistance(double JD, PlanetP object1, StelObjectP object2, int mode);
	double findInitialStep(double startJD, double stopJD, QStringList objects);
	bool findPrecise(QPair<double, double>* out, PlanetP object1, StelObjectP object2, double JD, double step, int prevSign, int mode);
	void fillPhenomenaTable(const QMap<double, double> list, const PlanetP object1, const StelObjectP object2, int mode);
	void fillPhenomenaTable(const QMap<double, double> list, const PlanetP object1, const NebulaP object2);
	//! @note modes: 0 - conjuction, 1 - opposition, 2 - greatest elongation
	void fillPhenomenaTable(const QMap<double, double> list, const PlanetP object1, const PlanetP object2, int mode);
	void fillPhenomenaTableVis(QString phenomenType, double JD, QString firstObjectName, float firstObjectMagnitude,
				   QString secondObjectName, float secondObjectMagnitude, QString separation, QString elongation,
				   QString angularDistance, QString elongTooltip="", QString angDistTooltip="");
	//! Calculation greatest elongations
	QMap<double, double> findGreatestElongationApproach(PlanetP& object1, StelObjectP& object2, double startJD, double stopJD);
	bool findPreciseGreatestElongation(QPair<double, double>* out, PlanetP object1, StelObjectP object2, double JD, double stopJD, double step);
	//! Calculation stationary points
	QMap<double, double> findStationaryPointApproach(PlanetP& object1, double startJD, double stopJD);
	bool findPreciseStationaryPoint(QPair<double, double>* out, PlanetP object, double JD, double stopJD, double step, bool retrograde);
	double findRightAscension(double JD, PlanetP object);

	bool plotAltVsTime, plotAltVsTimeSun, plotAltVsTimeMoon, plotAltVsTimePositive, plotMonthlyElevation, plotMonthlyElevationPositive, plotDistanceGraph, plotAngularDistanceGraph, plotAziVsTime;
	int altVsTimePositiveLimit, monthlyElevationPositiveLimit;
	QString delimiter, acEndl;
	QStringList ephemerisHeader, phenomenaHeader, positionsHeader, wutHeader;
	static float brightLimit;
	static double minY, maxY, minYme, maxYme, minYsun, maxYsun, minYmoon, maxYmoon, transitX, minY1, maxY1, minY2, maxY2,
			     minYld, maxYld, minYad, maxYad, minYadm, maxYadm, minYaz, maxYaz;
	static QString yAxis1Legend, yAxis2Legend;

	//! Make sure that no tabs icons are outside of the viewport.
	//! @todo Limit the width to the width of the screen *available to the window*.
	void updateTabBarListWidgetWidth();

	void enableVisibilityAngularLimits(bool visible);

	//! Set clicked time in AstroCalc AltVSTime/AziVsTime graphs
	void setClickedTime(double posx);

	//! Memorize day for detecting rollover to next/prev one
	int oldGraphJD;

	//! Remember to redraw active plot when dialog becomes visible
	bool graphPlotNeedsRefresh;
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
				a = dso.capturedTexts().at(2).toInt();
			if (a==0 && mp.exactMatch(text(column)))
				a = mp.capturedTexts().at(1).toInt();			
			if (dso.exactMatch(other.text(column)))
				b = dso.capturedTexts().at(2).toInt();
			if (b==0 && mp.exactMatch(other.text(column)))
				b = mp.capturedTexts().at(1).toInt();
			if (a>0 && b>0)
				return a < b;
			else
				return text(column).toLower() < other.text(column).toLower();
		}
		else if (column == AstroCalcDialog::CColumnRA || column == AstroCalcDialog::CColumnDec)
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

		if (column == AstroCalcDialog::PhenomenaSeparation || column == AstroCalcDialog::PhenomenaElongation || column == AstroCalcDialog::PhenomenaAngularDistance)
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
				a = dso.capturedTexts().at(2).toInt();
			if (a==0 && mp.exactMatch(text(column)))
				a = mp.capturedTexts().at(1).toInt();
			if (dso.exactMatch(other.text(column)))
				b = dso.capturedTexts().at(2).toInt();
			if (b==0 && mp.exactMatch(other.text(column)))
				b = mp.capturedTexts().at(1).toInt();
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
		else
		{
			return text(column).toLower() < other.text(column).toLower();
		}
	}
};

#endif // ASTROCALCDIALOG_HPP
