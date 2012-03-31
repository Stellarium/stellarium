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
#include "StelDialog.hpp"
#include "Satellites.hpp"

class Ui_satellitesDialog;
class QTimer;
class SatellitesImportDialog;

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
	void refreshUpdateValues(void);

private slots:
	void listSatelliteGroup(int index);
	//! Reloads the satellites list with the currently selected group.
	void reloadSatellitesList();
	void updateSelectedSatelliteInfo(QListWidgetItem* cur, QListWidgetItem* prev);
	void saveSatellites(void);
	void setUpdateValues(int hours);
	void setUpdatesEnabled(int checkState);
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
	void satelliteDoubleClick(QListWidgetItem* item);
	void setOrbitParams(void);
	void updateTLEs(void);

private:
	void connectSatelliteGuiForm(void);
	void disconnectSatelliteGuiForm(void);

	Ui_satellitesDialog* ui;
	bool satelliteModified;
	void setAboutHtml(void);
	void updateGuiFromSettings(void);
	void populateGroupsList();
	QTimer* updateTimer;
	
	SatellitesImportDialog* importWindow;
};

#endif // _SATELLITESDIALOG_HPP_
