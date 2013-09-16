/*
 * Stellarium
 * Copyright (C) 2008 Guillaume Chereau
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
 
#ifndef _LOCATIONDIALOG_HPP_
#define _LOCATIONDIALOG_HPP_

#include <QObject>
#include "StelDialog.hpp"

class Ui_locationDialogForm;
class QModelIndex;
class StelLocation;

class LocationDialog : public StelDialog
{
	Q_OBJECT
public:
	LocationDialog();
	virtual ~LocationDialog();
	//! Notify that the application style changed
	void styleChanged();

public slots:
	void retranslate();

protected:
	//! Initialize the dialog widgets and connect the signals/slots
	virtual void createDialogContent();
	Ui_locationDialogForm* ui;
	
private:
	//! Set the values of all the fields from a location info
	//! Also move the observer to this position
	void setFieldsFromLocation(const StelLocation& loc);
	
	//! Create a StelLocation instance from the fields
	StelLocation locationFromFields() const;
	
	//! True if the user is currently editing a new location
	bool isEditingNew;
	
	void disconnectEditSignals();
	void connectEditSignals();
	
	//! Update the map for the given location.
	void setMapForLocation(const StelLocation& loc);

	//! Populates the drop-down list of planets.
	//! The displayed names are localized in the current interface language.
	//! The original names are kept in the user data field of each QComboBox
	//! item.
	void populatePlanetList();

	//! Populates the drop-down list of countries.
	//! The displayed names are localized in the current interface language.
	//! The original names are kept in the user data field of each QComboBox
	//! item.
	void populateCountryList();
	
private slots:
	//! To be called when user edits any field
	void reportEdit();
	
	//! Update the widget to make sure it is synchrone if the location is changed programmatically
	//! This function should be called repeatidly with e.g. a timer
	void updateFromProgram(const StelLocation& location);
	
	//! Called when the map is clicked.
	void setPositionFromMap(double longitude, double latitude);
	
	//! Called when the user activates an item from the locations list.
	void setPositionFromList(const QModelIndex& index);
	
	//! Called when the planet is manually changed.
	void moveToAnotherPlanet(const QString& text);
	//! Called when latitude/longitude/altitude is modified
	void setPositionFromCoords(int i=0);
	
	//! Called when the user clicks on the add to list button
	void addCurrentLocationToList();
	
	//! Called when the user clicks on the delete button
	void deleteCurrentLocationFromList();
	
	//! Called when the user wants to use the current location as default
	void setDefaultLocation();
	
private:
	QString lastPlanet;	
	bool lastVisionMode;
	
	//! Updates the check state and the enabled/disabled status.
	void updateDefaultLocationControls(bool currentIsDefault);
};

#endif // _LOCATIONDIALOG_HPP_
