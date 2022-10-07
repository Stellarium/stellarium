/*
 * Stellarium Telescope Control Plug-in
 * 
 * Copyright (C) 2009-2011 Bogdan Marinov (this file)
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
 
#ifndef TELESCOPEDIALOG_HPP
#define TELESCOPEDIALOG_HPP

#include <QObject>
#include <QHash>
#include <QIntValidator>
#include <QModelIndex>
#include <QStandardItemModel>

#include "StelDialog.hpp"
#include "TelescopeControl.hpp"
#include "TelescopeConfigurationDialog.hpp"

class Ui_telescopeDialogForm;
class TelescopeConfigurationDialog;
class TelescopeControl;

class TelescopeDialog : public StelDialog
{
	Q_OBJECT
public:
	TelescopeDialog(const QString &dialogName=QString("TelescopeDialog"), QObject* parent=nullptr);
	~TelescopeDialog() override;
	void updateStyle();

public slots:
	void retranslate() override;

protected:
	//! Initialize the dialog widgets and connect the signals/slots
	void createDialogContent() override;
	Ui_telescopeDialogForm* ui;
	
private:
	//! Update the text and the tooltip of the ChangeStatus button
	void updateStatusButtonForSlot(int slot);
	
	void setStatusButtonToStart();
	void setStatusButtonToStop();
	void setStatusButtonToConnect();
	void setStatusButtonToDisconnect();
	
	void setAboutText();
	void setHeaderNames();
	void updateWarningTexts();
	
	QString getTypeLabel(TelescopeControl::ConnectionType type);
	void addModelRow(int slotNumber, TelescopeControl::ConnectionType type, TelescopeControl::TelescopeStatus status, const QString& name);
	void updateModelRow(int rowNumber, TelescopeControl::ConnectionType type, TelescopeControl::TelescopeStatus status, const QString& name);
	
private slots:
	void buttonChangeStatusPressed(void);
	void buttonConfigurePressed(void);
	void buttonAddPressed(void);
	void buttonRemovePressed(void);
	
	void buttonBrowseServerDirectoryPressed(void);
	
	//! Slot for receiving information from TelescopeConfigurationDialog
	void saveChanges(QString name, TelescopeControl::ConnectionType type);
	//! Slot for receiving information from TelescopeConfigurationDialog
	void discardChanges(void);
	
	void selectTelecope(const QModelIndex &);
	void configureTelescope(const QModelIndex &);
	
	//! Update the list of telescopes with their current states.
	//! This is called every 200ms by a QTimer!
	// FIXME! This should be triggered by signals from the telescopes!
	void updateTelescopeStates(void);

private:
	//! @enum ModelColumns This enum defines the number and the order of the columns in the table that lists active telescopes
	enum ModelColumns {
		ColumnSlot = 0,		//!< slot number column
		//ColumnStartup,	//!< startup checkbox column
		ColumnStatus,		//!< telescope status column
		ColumnType,		//!< telescope type column
		ColumnName,		//!< telescope name column
		ColumnCount		//!< total number of columns
	};
	
	TelescopeControl * telescopeManager;
	QMap<int, QString> statusString;
	TelescopeConfigurationDialog configurationDialog;
	QStandardItemModel * telescopeListModel;

	TelescopeControl::TelescopeStatus telescopeStatus[TelescopeControl::SLOT_NUMBER_LIMIT];
	TelescopeControl::ConnectionType connectionTypes[TelescopeControl::SLOT_NUMBER_LIMIT];
	
	int telescopeCount;
	int configuredSlot;
	bool configuredTelescopeIsNew;
};

#endif // TELESCOPEDIALOG_HPP
