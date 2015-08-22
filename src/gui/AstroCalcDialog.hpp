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

#include "StelDialog.hpp"
#include "StelCore.hpp"

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

	//! If a current planetary position is selected by user, the current date change and the object is selected.
	void selectCurrentPlanetaryPosition(const QModelIndex &modelIndex);

	//! Calculate ephemeris for selected celestial body and fill the list.
	void generateEphemeris();

	//! If an ephemride is selected by user, the current date change and the object is selected.
	void selectCurrentEphemeride(const QModelIndex &modelIndex);

private:
	class StelCore* core;
	class SolarSystem* solarSystem;
	class StelObjectMgr* objectMgr;

	//! Update header names for planetary positions table
	void setPlanetaryPositionsHeaderNames();
	//! Update header names for ephemeris table
	void setEphemerisHeaderNames();
	//! Update description of the AstroCalc
	void setAstroCalcDescription();

	//! Init header and list of planetary positions
	void initListPlanetaryPositions();

	//! Init header and list of ephemeris
	void initListEphemeris();

	//! Populates the drop-down list of celestial bodies.
	//! The displayed names are localized in the current interface language.
	//! The original names are kept in the user data field of each QComboBox
	//! item.
	void populateCelestialBodyList();

	void populateEphemerisTimeStepsList();
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
