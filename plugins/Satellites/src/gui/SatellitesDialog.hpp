/*
 * Stellarium Satellites Plug-in GUI
 * 
 * Copyright (C) 2010 Matthew Gates
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
 
#ifndef _SATELLITESDIALOG_HPP_
#define _SATELLITESDIALOG_HPP_

#include <QObject>
#include <QModelIndex>
#include "StelDialog.hpp"
#include "Satellites.hpp"

class Ui_satellitesDialog;
class QListWidgetItem;
class QSortFilterProxyModel;
class QStandardItemModel;
class QTimer;

class SatellitesImportDialog;
class SatellitesListFilterModel;

class SatellitesDialog : public StelDialog
{
	Q_OBJECT

public:
	SatellitesDialog();
	~SatellitesDialog();

protected:
	//! Initialize the dialog widgets and connect the signals/slots
	void createDialogContent();

public slots:
	void retranslate();

private slots:
	//! Update the countdown to the next update.
	void updateCountdown();
	//! Filter the satellites list according to the selected (pseudo)group.
	//! @param index selection index of the groups drop-down list.
	void filterListByGroup(int index);
	//! Populates the satellites list with the currently selected group.
	void reloadSatellitesList();
	void updateSelectedInfo(const QModelIndex& cur, const QModelIndex& prev);
	void saveSatellites(void);
	void updateStateReceiver(Satellites::UpdateState state);
	void updateCompleteReceiver(int numUpdated, int total, int missing);
	void sourceEditingDone(void);
	void saveSourceList(void);
	void deleteSourceRow(void);
	void addSourceRow(void);
	void restoreDefaults(void);
	void saveSettings(void);
	void addSatellites(const TleDataList& newSatellites);
	void removeSatellites();
	void setDisplayFlag(bool display);
	void setOrbitFlag(bool display);
	void handleDoubleClick(const QModelIndex & index);
	void setOrbitParams(void);
	void updateTLEs(void);

private:
	void connectSatelliteGuiForm(void);
	void disconnectSatelliteGuiForm(void);
	void setAboutHtml();
	//! Update the Settings tab with values from the plug-in.
	//! Calls updateCountdown(). Connected to Satellites::settingsChanged().
	void updateSettingsPage();
	//! Populates the list of satellite groups on the %Satellites tab.
	void populateGroupsList();
	//! Populates the list of sources on the Sources tab.
	void populateSourcesList();
	
	Ui_satellitesDialog* ui;
	bool satelliteModified;
	
	QTimer* updateTimer;
	
	SatellitesImportDialog* importWindow;
	
	SatellitesListFilterModel* filterModel;
};

#endif // _SATELLITESDIALOG_HPP_
