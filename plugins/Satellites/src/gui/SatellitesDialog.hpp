/*
 * Stellarium Satellites Plug-in GUI
 * 
 * Copyright (C) 2010 Matthew Gates
 * Copyright (C) 2015 Nick Fedoseev (Iridium flares)
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
 
#ifndef SATELLITESDIALOG_HPP
#define SATELLITESDIALOG_HPP

#include <QObject>
#include <QModelIndex>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include "StelDialog.hpp"
#include "Satellites.hpp"

class Ui_satellitesDialog;
class QCheckBox;
class QListWidgetItem;
class QSortFilterProxyModel;
class QStandardItemModel;
class QTimer;

class SatellitesImportDialog;
class SatellitesListFilterModel;

//! Main configuration window of the %Satellites plugin.
//! @todo Save sources list on check/uncheck.
//! @ingroup satellites
class SatellitesDialog : public StelDialog
{
	Q_OBJECT

public:
#if(SATELLITES_PLUGIN_IRIDIUM == 1)
	//! Defines the number and the order of the columns in the Iridium Flares table
	//! @enum IridiumFlaresColumns
	enum IridiumFlaresColumns {
		IridiumFlaresDate,	//! date and time of Iridium flare
		IridiumFlaresMagnitude,	//! magnitude of the flare
		IridiumFlaresAltitude,	//! altitude of the flare
		IridiumFlaresAzimuth,	//! azimuth of the flare
		IridiumFlaresSatellite,	//! satellite name
		IridiumFlaresCount	//! total number of columns
	};
#endif

	SatellitesDialog();
	~SatellitesDialog();

protected:
	//! Initialize the dialog widgets and connect the signals/slots
	void createDialogContent();

public slots:
	void retranslate();

private slots:
	void jumpToSourcesTab();
	//! Update the countdown to the next update.
	void updateCountdown();
	//! Filter the satellites list according to the selected (pseudo)group.
	//! @param index selection index of the groups drop-down list.
	void filterListByGroup(int index);
	//! Populate the satellite data fields from the selected satellite(s).
	//! @note The previous version used data only from the @em current item
	//! in the list, not the whole selection. (Qt makes a difference between
	//! "@em the current" and "@em a selected" item - a selection can contain
	//! multiple items.)
	void updateSatelliteData();
	void updateSatelliteAndSaveData();
	void saveSatellites(void);
	void showUpdateState(Satellites::UpdateState state);
	void showUpdateCompleted(int updated, int total, int added, int missing);
	
	//! @name Sources Tab 
	//@{
	void saveEditedSource();
	void updateButtonsProperties();
	void saveSourceList(void);
	void deleteSourceRow(void);
	void addSourceRow(void);
	void editSourceRow(void);
	//! Toggle between modes in the Sources list.
	//! If automatic adding is enabled, items in the list become checkable.
	void toggleCheckableSources();
	//@}
	
	void restoreDefaults(void);
	void saveSettings(void);
	void addSatellites(const TleDataList& newSatellites);
	void removeSatellites();
	//! Apply the "Displayed" and "Orbit" boxes to the selected satellite(s).
	void setFlags();
	//! Find out if a group is added or toggled in the group selector.
	void handleGroupChanges(QListWidgetItem* item);
	//! Display, select and start tracking the double clicked satellite.
	void trackSatellite(const QModelIndex & index);
	void updateTLEs(void);

#if(SATELLITES_PLUGIN_IRIDIUM == 1)
	void predictIridiumFlares();
	void selectCurrentIridiumFlare(const QModelIndex &modelIndex);
	void savePredictedIridiumFlares();
#endif

	void searchSatellitesClear();

	// change selection's color
	void askSatMarkerColor();
	void askSatOrbitColor();
	void askSatInfoColor();

	// change description text
	void descriptionTextChanged();

	void setRightSideToROMode();
	void setRightSideToRWMode();

private:
	//! @todo find out if this is really necessary... --BM
	void enableSatelliteDataForm(bool enabled);
	void populateAboutPage();
	//! Update the Settings tab with values from the plug-in.
	//! Calls updateCountdown(). Connected to Satellites::settingsChanged().
	void updateSettingsPage();
	//! Populates the satellite groups filtering menu on the %Satellites tab.
	//! Preserves the current item, if it's still in the new list.
	void populateFilterMenu();
	//! Populates the list of sources on the Sources tab.
	void populateSourcesList();	
	void populateInfo();	
	//! Add the special "New group..." editable item to the group selector.
	//! Unlike the other items, which can only be checked/unchecked, this one
	//! can be edited. Saving the edit will add a new group with the specified
	//! name.
	//! Called by updateSatelliteData() and handleGroupChanges().
	void addSpecialGroupItem();
	//! Applies the changes in the group selector to the selected satellites.
	void setGroups();

#if(SATELLITES_PLUGIN_IRIDIUM == 1)
	//! Update header names for Iridium flares table
	void setIridiumFlaresHeaderNames();

	//! Init header and list of Iridium flares
	void initListIridiumFlares();
#endif

	Ui_satellitesDialog* ui;
	bool satelliteModified;
	
	QTimer* updateTimer;
	
	SatellitesImportDialog* importWindow;
	
	SatellitesListFilterModel* filterModel;
	
	//! Makes sure that newly added source lines are as checkable as the rest.
	Qt::ItemDataRole checkStateRole;

	QString delimiter;
#if(SATELLITES_PLUGIN_IRIDIUM == 1)
	QStringList iridiumFlaresHeader;
#endif

	// colorpickerbutton's color
	QColor buttonMarkerColor, buttonOrbitColor, buttonInfoColor;

	static const QString dash;
};

#if(SATELLITES_PLUGIN_IRIDIUM == 1)
// Reimplements the QTreeWidgetItem class to fix the sorting bug
class SatPIFTreeWidgetItem : public QTreeWidgetItem
{
public:
	SatPIFTreeWidgetItem(QTreeWidget* parent)
		: QTreeWidgetItem(parent)
	{
	}

private:
	bool operator < (const QTreeWidgetItem &other) const
	{
		int column = treeWidget()->sortColumn();

		if (column == SatellitesDialog::IridiumFlaresMagnitude)
		{
			return text(column).toFloat() < other.text(column).toFloat();
		}
		else
		{
			return text(column).toLower() < other.text(column).toLower();
		}
	}
};
#endif

#endif // SATELLITESDIALOG_HPP
