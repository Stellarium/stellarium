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

#ifndef _ASTROCALCDIALOG_HPP_
#define _ASTROCALCDIALOG_HPP_

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

class AstroCalcDialog : public StelDialog
{
	Q_OBJECT

public:
	//! Defines the number and the order of the columns in the table that lists celestial bodies positions
	//! @enum CPositionsColumns
	enum CPositionsColumns {
		CColumnName,		//! name of object
		CColumnRA,		//! right ascension
		CColumnDec,		//! declination
		CColumnMagnitude,	//! magnitude
		CColumnAngularSize,	//! angular size
		CColumnExtra,		//! extra data (surface brightness, separation, period, etc.)		
		CColumnType,		//! type of object
		CColumnCount		//! total number of columns
	};

	//! Defines the number and the order of the columns in the ephemeris table
	//! @enum EphemerisColumns
	enum EphemerisColumns {
		EphemerisDate,		//! date and time of ephemeris
		EphemerisJD,		//! JD
		EphemerisRA,		//! right ascension
		EphemerisDec,		//! declination
		EphemerisMagnitude,	//! magnitude
		EphemerisPhase,		//! phase
		EphemerisDistance,	//! distance
		EphemerisElongation,	//! elongation
		EphemerisCount		//! total number of columns
	};

	//! Defines the number and the order of the columns in the phenomena table
	//! @enum PhenomenaColumns
	enum PhenomenaColumns {
		PhenomenaType,		//! type of phenomena
		PhenomenaDate,		//! date and time of ephemeris		
		PhenomenaObject1,	//! first object
		PhenomenaObject2,	//! second object
		PhenomenaSeparation,	//! angular separation
		PhenomenaCount		//! total number of columns
	};

	//! Defines the type of graphs
	//! @enum GraphsTypes
	enum GraphsTypes {
		GraphMagnitudeVsTime	= 1,
		GraphPhaseVsTime	= 2,
		GraphDistanceVsTime	= 3,
		GraphElongationVsTime	= 4,
		GraphAngularSizeVsTime	= 5,
		GraphPhaseAngleVsTime	= 6
	};

	AstroCalcDialog(QObject* parent);
	virtual ~AstroCalcDialog();

	//! Notify that the application style changed
	void styleChanged();

	static QVector<Vec3d> EphemerisListCoords;
	static QVector<QString> EphemerisListDates;
	static QVector<float> EphemerisListMagnitudes;
	static int DisplayedPositionIndex;

public slots:
        void retranslate();

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

	//! Calculate phenomena for selected celestial body and fill the list.
	void calculatePhenomena();
	void cleanupPhenomena();
	void selectCurrentPhenomen(const QModelIndex &modelIndex);
	void savePhenomena();
	void savePhenomenaAngularSeparation(double v);

	void savePhenomenaCelestialBody(int index);
	void savePhenomenaCelestialGroup(int index);
	void savePhenomenaOppositionFlag(bool b);

	//! Compute planetary data
	void saveFirstCelestialBody(int index);
	void saveSecondCelestialBody(int index);
	void computePlanetaryData();

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

	void saveGraphsCelestialBody(int index);
	void saveGraphsFirstId(int index);
	void saveGraphsSecondId(int index);
	void drawXVsTimeGraphs();

	void drawMonthlyElevationGraph();
	void updateMonthlyElevationTime();
	void syncMonthlyElevationTime();

	// WUT
	void saveWutMagnitudeLimit(double mag);
	void saveWutTimeInterval(int index);
	void calculateWutObjects();
	void selectWutObject();
	void saveWutObjects();

	void updateAstroCalcData();

	void changePage(QListWidgetItem *current, QListWidgetItem *previous);

	void updateSolarSystemData();

private:
	class StelCore* core;
	class SolarSystem* solarSystem;
	class NebulaMgr* dsoMgr;
	class StarMgr* starMgr;
	class StelObjectMgr* objectMgr;
	class StelLocaleMgr* localeMgr;
	class StelMovementMgr* mvMgr;
	QSettings* conf;
	QTimer *currentTimeLine;
	QHash<QString,QString> wutObjects;
	QHash<QString,int> wutCategories;

	//! Update header names for celestial positions tables
	void setCelestialPositionsHeaderNames();
	//! Update header names for ephemeris table
	void setEphemerisHeaderNames();
	//! Update header names for phenomena table
	void setPhenomenaHeaderNames();

	//! Init header and list of celestial positions
	void initListCelestialPositions();
	//! Init header and list of ephemeris
	void initListEphemeris();
	//! Init header and list of phenomena
	void initListPhenomena();

	//! Populates the drop-down list of celestial bodies.
	//! The displayed names are localized in the current interface language.
	//! The original names are kept in the user data field of each QComboBox
	//! item.
	void populateCelestialBodyList();	
	//! Populates the drop-down list of time steps.
	void populateEphemerisTimeStepsList();
	//! Populates the drop-down list of major planets.
	void populateMajorPlanetList();
	//! Populates the drop-down list of groups of celestial bodies.
	void populateGroupCelestialBodyList();	
	//! Prepare graph settings
	void prepareAxesAndGraph();
	void prepareXVsTimeAxesAndGraph();
	void prepareMonthlyEleveationAxesAndGraph();
	//! Populates the drop-down list of time intervals for WUT tool.
	void populateTimeIntervalsList();
	//! Populates the list of groups for WUT tool.
	void populateWutGroups();

	void populateFunctionsList();

	//! Calculation conjunctions and oppositions.
	//! @note Ported from KStars, should be improved, because this feature calculate
	//! angular separation ("conjunction" defined as equality of right ascension
	//! of two body) and current solution is not accurate and slow.
	QMap<double, double> findClosestApproach(PlanetP& object1, PlanetP& object2, double startJD, double stopJD, double maxSeparation, bool opposition);
	double findDistance(double JD, PlanetP object1, PlanetP object2, bool opposition);
	bool findPrecise(QPair<double, double>* out, PlanetP object1, PlanetP object2, double JD, double step, int prevSign, bool opposition);
	void fillPhenomenaTable(const QMap<double, double> list, const PlanetP object1, const PlanetP object2, bool opposition);

	QMap<double, double> findClosestApproach(PlanetP& object1, NebulaP& object2, double startJD, double stopJD, double maxSeparation);
	double findDistance(double JD, PlanetP object1, NebulaP object2);
	bool findPrecise(QPair<double, double>* out, PlanetP object1, NebulaP object2, double JD, double step, int prevSign);
	void fillPhenomenaTable(const QMap<double, double> list, const PlanetP object1, const NebulaP object2);

	QMap<double, double> findClosestApproach(PlanetP& object1, StelObjectP& object2, double startJD, double stopJD, double maxSeparation);
	double findDistance(double JD, PlanetP object1, StelObjectP object2);
	bool findPrecise(QPair<double, double>* out, PlanetP object1, StelObjectP object2, double JD, double step, int prevSign);
	void fillPhenomenaTable(const QMap<double, double> list, const PlanetP object1, const StelObjectP object2);

	bool plotAltVsTime, plotMonthlyElevation, plotAltVsTimeSun, plotAltVsTimeMoon;
	QString delimiter, acEndl;
	QStringList ephemerisHeader, phenomenaHeader, positionsHeader;	
	static float brightLimit;
	static float minY, maxY, minYme, maxYme, minYsun, maxYsun, minYmoon, maxYmoon, transitX, minY1, maxY1, minY2, maxY2;
	static QString yAxis1Legend, yAxis2Legend;

	//! Make sure that no tabs icons are outside of the viewport.
	//! @todo Limit the width to the width of the screen *available to the window*.
	void updateTabBarListWidgetWidth();
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

		if (column == AstroCalcDialog::EphemerisRA || column == AstroCalcDialog::EphemerisDec)
		{
			return StelUtils::getDecAngle(text(column)) < StelUtils::getDecAngle(other.text(column));
		}
		else if (column == AstroCalcDialog::EphemerisMagnitude || column == AstroCalcDialog::EphemerisJD)
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

		if (column == AstroCalcDialog::PhenomenaSeparation)
		{
			return StelUtils::getDecAngle(text(column)) < StelUtils::getDecAngle(other.text(column));
		}
		else
		{
			return text(column).toLower() < other.text(column).toLower();
		}
	}
};

#endif // _ASTROCALCDIALOG_HPP_
