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
 
#ifndef _SLEWDIALOG_HPP_
#define _SLEWDIALOG_HPP_

#include <QObject>
#include <QHash>
#include <QString>
#include "StelStyle.hpp"

#include "StelDialog.hpp"

#include "StelObjectMgr.hpp"

class Ui_slewDialog;
class TelescopeControl;

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

private:
	TelescopeControl * telescopeManager;

	QHash<QString, int> connectedSlotsByName;

	void updateTelescopeList();
	void updateTelescopeControls();
};

#endif // _SLEWDIALOG_
