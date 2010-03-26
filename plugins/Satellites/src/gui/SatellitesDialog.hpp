/*
 * Stellarium Satellites Plug-in GUI
 * 
 * Copyright (C) 2009 Matthew Gates
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
 
#ifndef _SATELLITESDIALOG_HPP_
#define _SATELLITESDIALOG_HPP_

#include <QObject>
#include "StelDialogPlugin.hpp"
#include "Satellites.hpp"

class Ui_satellitesDialog;
class QTimer;

class SatellitesDialog : public StelDialogPlugin
{
	Q_OBJECT

public:
	SatellitesDialog();
	virtual ~SatellitesDialog();
	virtual void languageChanged();
	virtual void setStelStyle(const StelStyle& style);

protected:
	//! Initialize the dialog widgets and connect the signals/slots
	virtual void createDialogContent();
	Ui_satellitesDialog* ui;

public slots:
	void refreshUpdateValues(void);
	void close(void);

private slots:
	void groupFilterChanged(int index);
	void selectedSatelliteChanged(const QString& id);
	void saveSatellite(const QString& id);
	void setUpdateValues(int hours);
	void setUpdatesEnabled(bool b);
	void updateStateReceiver(Satellites::UpdateState state);
	void updateCompleteReceiver(int numUpdated);
	void sourceEditingDone(void);
	void saveSourceList(void);
	void deleteSourceRow(void);
	void addSourceRow(void);
	void restoreDefaults(void);
	void saveSettings(void);
	void showSelectedSatellites(void);
	void hideSelectedSatellites(void);
	void visibleCheckChanged(int state);
	void satelliteDoubleClick(QListWidgetItem* item);

private:
	bool satelliteModified;
	void setAboutHtml(void);
	void updateGuiFromSettings(void);
	QTimer* updateTimer;

};

#endif // _SATELLITESDIALOG_HPP_
