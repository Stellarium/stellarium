/*
 * Stellarium
 * Copyright (C) 2008 Fabien Chereau
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
 
#ifndef _CONFIGURATIONDIALOG_HPP_
#define _CONFIGURATIONDIALOG_HPP_

#include <QObject>
#include "StelDialog.hpp"

class Ui_configurationDialogForm;

class ConfigurationDialog : public StelDialog
{
	Q_OBJECT;
public:
	ConfigurationDialog();
	virtual ~ConfigurationDialog();
	void languageChanged();
	//! Notify that the application style changed
	void styleChanged();
protected:
	//! Initialize the dialog widgets and connect the signals/slots
	virtual void createDialogContent();	
	Ui_configurationDialogForm* ui;

private slots:
	void setNoSelectedInfo(void);
	void setAllSelectedInfo(void);
	void setBriefSelectedInfo(void);
	void languageChanged(const QString& languageCode);
	void setStartupTimeMode(void);
	void setDiskViewport(bool);
	void setSphericMirror(bool);
	void cursorTimeOutChanged();
	void cursorTimeOutChanged(double d) {cursorTimeOutChanged();}
	
	//! Save the current viewing option including landscape, location and sky culture
	//! This doesn't include the current viewing direction, time and FOV since those have specific controls
	void saveCurrentViewOptions();
	
	//! Reset all stellarium options.
	//! This basically replaces the config.ini by the default one
	void resetAllOptions();
	
};

#endif // _CONFIGURATIONDIALOG_HPP_
