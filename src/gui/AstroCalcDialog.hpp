/*
 * Stellarium
 * 
 * Copyright (C) 2015 Alexander Wolf
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

#include "StelDialog.hpp"
#include "StelCore.hpp"
#include "Planet.hpp"

class Ui_astroCalcDialogForm;

class AstroCalcDialog : public StelDialog
{
	Q_OBJECT

public:
	//! Defines the number and the order of the columns in the table that lists planetary positions
	//! @enum PlanetaryPositionsColumns
	enum PlanetaryPositionsColumns {
		ColumnName,		//! name of object
		ColumnRA,		//! right ascension
		ColumnDec,		//! declination
		ColumnMagnitude,	//! magnitude
		ColumnType,		//! type of object
		ColumnCount		//! total number of columns
	};

	//! Defines the number and the order of the columns in the ephemeris table
	//! @enum EphemerisColumns
	enum EphemerisColumns {
		EphemerisDate,		//! date and time of ephemeris
		EphemerisJD,		//! JD
		EphemerisRA,		//! right ascension
		EphemerisDec,		//! declination
		EphemerisMagnitude,	//! magnitude
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

	AstroCalcDialog();
	virtual ~AstroCalcDialog();

public slots:
        void retranslate();

protected:
        //! Initialize the dialog widgets and connect the signals/slots.
        virtual void createDialogContent();
        Ui_astroCalcDialogForm *ui;

private slots:
	//! Search planetary positions and fill the list.
	void currentPlanetaryPositions();
	void selectCurrentPlanetaryPosition(const QModelIndex &modelIndex);

	//! Calculate ephemeris for selected celestial body and fill the list.
	void generateEphemeris();
	void selectCurrentEphemeride(const QModelIndex &modelIndex);

	//! Calculate phenomena for selected celestial body and fill the list.
	void calculatePhenomena();
	void selectCurrentPhenomen(const QModelIndex &modelIndex);

private:
	class StelCore* core;
	class SolarSystem* solarSystem;
	class StelObjectMgr* objectMgr;

	//! Update header names for planetary positions table
	void setPlanetaryPositionsHeaderNames();
	//! Update header names for ephemeris table
	void setEphemerisHeaderNames();
	//! Update header names for phenomena table
	void setPhenomenaHeaderNames();

	//! Init header and list of planetary positions
	void initListPlanetaryPositions();
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

	//! Calculation conjuctions and oppositions.
	//! @note Ported from KStars, should be improved, because this feature calculate
	//! angular separation ("conjunction" defined as equality of right ascension
	//! of two body) and current solution is not accurate and slow.
	QMap<double, double> findClosestApproach(PlanetP& object1, PlanetP& object2, double startJD, double stopJD, float maxSeparation, bool opposition);
	double findDistance(double JD, PlanetP object1, PlanetP object2, bool opposition);
	bool findPrecise(QPair<double, double>* out, PlanetP object1, PlanetP object2, double JD, double step, int prevSign, bool opposition);
	void fillPhenomenaTable(const QMap<double, double> list, const PlanetP object1, const PlanetP object2, bool opposition);
};

// Reimplements the QTreeWidgetItem class to fix the sorting bug
class TreeWidgetItem : public QTreeWidgetItem
{
public:
	TreeWidgetItem(QTreeWidget* parent)
		: QTreeWidgetItem(parent)
	{
	}

private:
	bool operator < (const QTreeWidgetItem &other) const
	{
		int column = treeWidget()->sortColumn();

		if (column == AstroCalcDialog::ColumnMagnitude)
			return text(column).toFloat() < other.text(column).toFloat();
		else
			return text(column).toLower() < other.text(column).toLower();

	}
};

#endif // _ASTROCALCDIALOG_HPP_
