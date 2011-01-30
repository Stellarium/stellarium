/*
 * Stellarium Telescope Control Plug-in
 * 
 * Copyright (C) 2009-2010 Bogdan Marinov (this file)
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
 
#ifndef _TELESCOPEDIALOG_HPP_
#define _TELESCOPEDIALOG_HPP_

#include <QObject>
#include <QHash>
#include <QIntValidator>
#include <QModelIndex>
#include <QStandardItemModel>

//#include "StelDialog.hpp"
#include "StelDialogTelescopeControl.hpp"
#include "TelescopeControlGlobals.hpp"
#include "TelescopeConfigurationDialog.hpp"

using namespace TelescopeControlGlobals;

class Ui_telescopeDialogForm;
class TelescopeConfigurationDialog;
class TelescopeControl;

class TelescopeDialog : public StelDialogTelescopeControl
{
	Q_OBJECT
public:
	TelescopeDialog();
	virtual ~TelescopeDialog();
	void updateStyle();

public slots:
	void languageChanged();

protected:
	//! Initialize the dialog widgets and connect the signals/slots
	virtual void createDialogContent();
	Ui_telescopeDialogForm* ui;
	
private:
	//! Update the text and the tooltip of the ChangeStatus button
	void updateStatusButtonForSlot(int slot);
	
	void setStatusButtonToStart();
	void setStatusButtonToStop();
	void setStatusButtonToConnect();
	void setStatusButtonToDisconnect();
	
private slots:
	void buttonChangeStatusPressed(void);
	void buttonConfigurePressed(void);
	void buttonAddPressed(void);
	void buttonRemovePressed(void);
	
	void checkBoxUseExecutablesToggled(bool);
	void buttonBrowseServerDirectoryPressed(void);
	
	//! Slot for receiving information from TelescopeConfigurationDialog
	void saveChanges(QString name, ConnectionType type);
	//! Slot for receiving information from TelescopeConfigurationDialog
	void discardChanges(void);
	
	void toggleReticles(int);
	void toggleLabels(int);
	void toggleCircles(int);
	void selectTelecope(const QModelIndex &);
	void configureTelescope(const QModelIndex &);
	
	//! Update the list of telescopes with their current states
	void updateTelescopeStates(void);

private:
	enum TelescopeStatus {
		StatusNA = 0,
		StatusStarting,
		StatusConnecting,
		StatusConnected,
		StatusDisconnected,
		StatusStopped,
		StatusCount
	};
	
	//! @enum ModelColumns This enum defines the number and the order of the columns in the table that lists active telescopes
	enum ModelColumns {
		ColumnSlot = 0,		//!< slot number column
		//ColumnStartup,	//!< startup checkbox column
		ColumnStatus,		//!< telescope status column
		ColumnType,		//!< telescope type column
		ColumnName,		//!< telescope name column
		ColumnCount		//!< total number of columns
	};
	
	QHash<int, QString> statusString;
	TelescopeConfigurationDialog configurationDialog;
	
	QStandardItemModel * telescopeListModel;
	
	TelescopeControl * telescopeManager;
	
	int telescopeStatus[SLOT_NUMBER_LIMIT];
	ConnectionType telescopeType[SLOT_NUMBER_LIMIT];
	
	int telescopeCount;
	int configuredSlot;
	bool configuredTelescopeIsNew;
};

#endif // _TELESCOPEDIALOG_
