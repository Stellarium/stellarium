/*
 * Stellarium Telescope Control Plug-in
 * 
 * Copyright (C) 2010 Bogdan Marinov (this file)
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
 
#ifndef SLEWDIALOG_HPP
#define SLEWDIALOG_HPP

#include <QObject>
#include <QHash>
#include <QString>
#include <QDir>

#include "StoredPointsDialog.hpp"

#include "StelStyle.hpp"
#include "StelDialog.hpp"
#include "StelFileMgr.hpp"

class Ui_slewDialog;
class TelescopeControl;
class StoredPointsDialog;
class TelescopeClient;

class SlewDialog : public StelDialog
{
	Q_OBJECT

public:
	SlewDialog();
	virtual ~SlewDialog();

public slots:
	void retranslate();
	
protected:
	//! Initialize the dialog widgets and connect the signals/slots
	virtual void createDialogContent();
	Ui_slewDialog* ui;
	
private slots:
	//! shows the configuration dialog of the plug-in (i.e. TelescopeDialog)
	void showConfiguration();

	//! reads the fields and slews a telescope
	void slew();
	//! reads the fields and sync a telescope
	void sync();

	void addTelescope(int slot, QString name);
	void removeTelescope(int slot);

	//! sets the format of the input fields to Hours-Minutes-Seconds.
	//! Sets the right ascension field to HMS and the declination field to DMS.
	//! The parameter is necessary for signal/slot compatibility (QRadioButton).
	//! If "set" is "false", this method does nothing.
	void setFormatHMS(bool set);
	//! sets the format of the input fields to Degrees-Minutes-Seconds.
	//! Sets both the right ascension field and the declination field to DMS.
	//! The parameter is necessary for signal/slot compatibility (QRadioButton).
	//! If "set" is "false", this method does nothing.
	void setFormatDMS(bool set);
	//! sets the format of the input fields to Decimal degrees.
	//! Sets both the right ascension field and the declination field
	//! to decimal degrees.
	//! The parameter is necessary for signal/slot compatibility (QRadioButton).
	//! If "set" is "false", this method does nothing.
	void setFormatDecimal(bool set);
	//! Sets the input fields to current info
	void getCurrentObjectInfo();
	//! Sets the input fields to current info
	void getCenterInfo();

	//! Add or remove user points
	void editStoredPoints();
	void addStoredPointToComboBox(int number, QString name, double radiansRA, double radiansDec);
	void removeStoredPointFromComboBox(int number);
	void clearStoredPointsFromComboBox();
	//! Sets the input fields to selected point info
	void getStoredPointInfo();

	void onMove(double angle, double speed);
	void onCurrentTelescopeChanged();

private:
	QSharedPointer<TelescopeClient> currentTelescope() const;
	void updateTelescopeList();
	void updateTelescopeControls();
	void updateStoredPointsList();
	void savePointsToFile();
	void loadPointsFromFile();

	TelescopeControl * telescopeManager =  Q_NULLPTR;
	StoredPointsDialog * storedPointsDialog = Q_NULLPTR;
	QHash<QString, int> connectedSlotsByName;
	QVariantMap storedPointsDescriptions;
};

#endif // SLEWDIALOG_HPP
