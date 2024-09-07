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
#include <QRegularExpressionValidator>
#include "StelDialog.hpp"
#include "TelescopeControl.hpp"

#if defined(Q_OS_WIN) && QT_VERSION<QT_VERSION_CHECK(6,0,0)
#include "../ASCOM/TelescopeClientASCOMWidget.hpp"
#endif

class Ui_telescopeConfigurationDialog;
class StelStyle;

class TelescopeConfigurationDialog : public StelDialog
{
	Q_OBJECT
public:
	TelescopeConfigurationDialog();
	~TelescopeConfigurationDialog() override;
	
	void initExistingTelescopeConfiguration(int slot);
	void initNewTelescopeConfiguration(int slot);

public slots:
	void retranslate() override;

protected:
	//! Initialize the dialog widgets and connect the signals/slots
	void createDialogContent() override;
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
	#if defined(Q_OS_WIN) && QT_VERSION<QT_VERSION_CHECK(6,0,0)
	void toggleTypeASCOM(bool enabled);
	#endif
	
	void deviceModelSelected(int modelIndex);


signals:
	void changesSaved(QString name, TelescopeControl::ConnectionType type);
	void changesDiscarded();
	
private:
	QStringList deviceModelNames;
	
	QRegularExpressionValidator * telescopeNameValidator;
	QRegularExpressionValidator * hostNameValidator;
	QRegularExpressionValidator * circleListValidator;
	QRegularExpressionValidator * serialPortValidator;

	#if defined(Q_OS_WIN) && QT_VERSION<QT_VERSION_CHECK(6,0,0)
	TelescopeClientASCOMWidget* ascomWidget;
	#endif
	
	int configuredSlot;
	
	TelescopeControl * telescopeManager;
};

#endif // TELESCOPECONFIGURATIONDIALOG_HPP
