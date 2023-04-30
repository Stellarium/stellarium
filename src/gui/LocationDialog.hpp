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
 
#ifndef LOCATIONDIALOG_HPP
#define LOCATIONDIALOG_HPP

#include <QObject>
#include "StelDialog.hpp"

class Ui_locationDialogForm;
class QModelIndex;
class QSortFilterProxyModel;
class QStringListModel;
class StelLocation;

class LocationDialog : public StelDialog
{
	Q_OBJECT
public:
	LocationDialog(QObject* parent);
	~LocationDialog() override;

public slots:
	//! Apply application style change
	void styleChanged(const QString &style) override;
	void retranslate() override;

protected:
	//! Initialize the dialog widgets and connect the signals/slots
	void createDialogContent() override;
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

	//! show or hide UI elements to edit location (Hide if on observer planet!)
	void setLocationUIvisible(bool visible);
	
	//! Update the map for the given location.
	void setMapForLocation(const StelLocation& loc);

	//! Populates the drop-down list of planets.
	//! The displayed names are localized in the current interface language.
	//! The original names are kept in the user data field of each QComboBox
	//! item.
	void populatePlanetList();

	//! Populates the drop-down list of regions.
	//! The displayed names are localized in the current interface language.
	//! The original names are kept in the user data field of each QComboBox
	//! item.
	void populateRegionList(const QString& planet = "");

	//! Populates the drop-down list of time zones.
	//! The displayed names are localized in the current interface language.
	//! The original names are kept in the user data field of each QComboBox
	//! item.
	void populateTimeZonesList();

	//! Populates tooltips for GUI elements.
	void populateTooltips();
	
private slots:
	//! Called whenever the StelLocationMgr is updated
	void reloadLocations();

	//! To be called when user edits any field
	void reportEdit();

	void saveTimeZone();

	//! Set timezone (to be connected to a signal from StelCore)
	//! This has to do some GUI element juggling.
	void setTimezone(QString tz);
	
	//! Update the widget to make sure it is synchrone if the location is changed programmatically
	//! This function should be called repeatidly with e.g. a timer
	void updateFromProgram(const StelLocation& newLocation);
	
	//! Called when the map is clicked.
	//! Creates new list for places nearby and feed into location list box.
	//! Sets lolcation to the clicked location
	//! Also signals (for locations on Earth only) to LandscapeMgr to load zero landscape with just the color of the clicked point.
	void setLocationFromMap(double longitude, double latitude, const QColor &color);
	
	//! Called when the user activates an item from the locations list.
	void setLocationFromList(const QModelIndex& index);
	
	//! Called when the planet is manually changed.
	void moveToAnotherPlanet();

	//! Called when latitude/longitude/altitude is modified
	//! The int argument is required by the Altitude spinbox signal connection, but unused.
	void setLocationFromCoords(int i=0);

	//! Called when the user clicks on the add to list button
	void addCurrentLocationToList();
	
	//! Called when the user clicks on the delete button
	void deleteCurrentLocationFromList();

	//! filter city list to show entries from single region only
	void filterSitesByRegion();

	//! reset city list to complete list (may have been reduced to picked list)
	void resetLocationList();

	//! called when the user wants get location from network.
	//! This is actually a toggle setting which will influence Stellarium's behaviour
	//! on next boot:
	//! @arg state true to immediately query location and activate auto-query on next launch.
	//! @arg state false to store current location as startup location.
	void ipQueryLocation(bool state);

	// Esp. for signals from StelSkyCultureMgr
	void populatePlanetList(QString) { populatePlanetList(); }

	void setDisplayFormatForSpins(bool flagDecimalDegrees);

#ifdef ENABLE_GPS
	//! called when the user wants to get GPS location from GPSD or directly attached (USB over virtual serial device) GPS device.
	//! The easiest and cleanest way to get GPS coordinates from a Linux device is via GPSD.
	//! On Windows (and Mac?), or where GPSD is not available, we must process the NMEA-183 messages and take care of the Serial port.
	//! The GPS connection stays open (blocking serial GPS device for other programs if not on GPSD) even with the dialog closed, until disabled again.
	//! @param enable true to start a repeating series of GPS queries, false to stop it.
	void gpsEnableQueryLocation(bool enable); // Can be toggled by QToolButton
	//! handle a few GUI elements when GPS query returns. Should be connected to LocationMgr's signal gpsResult().
	//! @param success true if location was found
	void gpsReturn(bool success);
	//! reset the default string after a short time where the button shows either success or failure of GPS data retrieval.
	//! To achieve this effect, this should be called by a QTimer.
	void resetGPSbuttonLabel();
#endif

	//! Called when the user wants to use the current location as default
	void setDefaultLocation(bool state);

	//! Updates the check state and the enabled/disabled status.
	void updateTimeZoneControls(bool useCustomTimeZone);

private:
	QString lastPlanet; // for caching when switching map
	QString customTimeZone;  // for caching when switching around timezones.
	QStringListModel* allModel;
	QStringListModel* pickedModel;
	QSortFilterProxyModel *proxyModel;
#ifdef ENABLE_GPS
	unsigned int gpsCount; // count received GPS positions (pure GUI eye candy)
#endif

	//QPixmap pixmap;

	//! Updates the check state and the enabled/disabled status.
	void updateDefaultLocationControls(bool currentIsDefault);
};

#endif // _LOCATIONDIALOG_HPP
