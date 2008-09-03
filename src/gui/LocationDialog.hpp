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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
 
#ifndef _LOCATIONDIALOG_HPP_
#define _LOCATIONDIALOG_HPP_

#include <QObject>
#include "StelDialog.hpp"

class Ui_locationDialogForm;
class QModelIndex;
class PlanetLocation;

class LocationDialog : public StelDialog
{
	Q_OBJECT;
public:
	LocationDialog();
	virtual ~LocationDialog();
	void languageChanged();
	//! Notify that the application style changed
	void styleChanged();
	
protected:
	//! Initialize the dialog widgets and connect the signals/slots
	virtual void createDialogContent();
	Ui_locationDialogForm* ui;
	
private:
	//! Set the values of all the fields from a location info
	//! Also move the observer to this position
	void setFieldsFromLocation(const PlanetLocation& loc);
	
	//! Create a PlanetLocation instance from the fields
	PlanetLocation locationFromFields() const;
	
	//! True if the user is currently editing a new location
	bool isEditingNew;
	
	//! To be called when user edits any field
	void reportEdit();
	
	void disconnectEditSignals();
	void connectEditSignals();
	
private slots:
	//! Called when the map is clicked
	void setPositionFromMap(double longitude, double latitude);
	
	//! Called when the user activates an item from the list
	void listItemActivated(const QModelIndex&);
	
	//! Called when the planet/country name is manually changed
	void comboBoxChanged(const QString& text);
	//! Called when latitude/longitude/altitude is modified
	void spinBoxChanged(int i=0);
	//! Called when the location name is manually changed
	void locationNameChanged(const QString& text);
	
	//! Called when the user clic on the save button
	void saveCurrentLocation();
};

#endif // _LOCATIONDIALOG_HPP_
