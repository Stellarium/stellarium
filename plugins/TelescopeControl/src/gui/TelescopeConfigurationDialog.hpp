/*
 * Stellarium TelescopeControl Plug-in
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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
*/
 
#ifndef TELESCOPECONFIGURATIONDIALOG_HPP
#define TELESCOPECONFIGURATIONDIALOG_HPP

#include <QObject>
#include <QHash>
#include <QIntValidator>
#include <QStringList>
#include "StelDialog.hpp"
#include "TelescopeControlGlobals.hpp"

#ifdef Q_OS_WIN
#include "../ASCOM/TelescopeClientASCOMWidget.hpp"
#endif

using namespace TelescopeControlGlobals;

class Ui_telescopeConfigurationDialog;
class TelescopeControl;
class StelStyle;

class TelescopeConfigurationDialog : public StelDialog
{
	Q_OBJECT
public:
	TelescopeConfigurationDialog();
	virtual ~TelescopeConfigurationDialog();
	
	void initExistingTelescopeConfiguration(int slot);
	void initNewTelescopeConfiguration(int slot);

public slots:
	void retranslate();

protected:
	//! Initialize the dialog widgets and connect the signals/slots
	virtual void createDialogContent();
	Ui_telescopeConfigurationDialog* ui;
	
private:
	QStringList* listSerialPorts();
	void initConfigurationDialog();
	void populateToolTips();
	
private slots:
	void buttonSavePressed();
	void buttonDiscardPressed();
	
	void toggleTypeLocal(bool);
	void toggleTypeConnection(bool);
	void toggleTypeVirtual(bool);
	void toggleTypeRTS2(bool);
	void toggleTypeINDI(bool enabled);
	#ifdef Q_OS_WIN
	void toggleTypeASCOM(bool enabled);
	#endif
	
	void deviceModelSelected(const QString&);


signals:
	void changesSaved(QString name, ConnectionType type);
	void changesDiscarded();
	
private:
	QStringList deviceModelNames;
	
	QRegExpValidator * telescopeNameValidator;
	QRegExpValidator * hostNameValidator;
	QRegExpValidator * circleListValidator;
	QRegExpValidator * serialPortValidator;

	#ifdef Q_OS_WIN
	TelescopeClientASCOMWidget* ascomWidget;
	#endif
	
	int configuredSlot;
	
	TelescopeControl * telescopeManager;
};

#endif // TELESCOPECONFIGURATIONDIALOG_HPP
