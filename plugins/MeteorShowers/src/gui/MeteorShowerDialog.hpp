/*
 * Stellarium Meteor Shower Plug-in GUI
 * 
 * Copyright (C) 2013 Marcos Cardinot
 * Copyright (C) 2011 Alexander Wolf
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
 
#ifndef _METEORSHOWERDIALOG_HPP_
#define _METEORSHOWERDIALOG_HPP_

#include <QObject>
#include "StelDialog.hpp"
#include "MeteorShowers.hpp"

class Ui_meteorShowerDialog;
class QTimer;

class MeteorShowerDialog : public StelDialog
{
	Q_OBJECT

public:
	MeteorShowerDialog();
	~MeteorShowerDialog();

protected:
	//! Initialize the dialog widgets and connect the signals/slots
	void createDialogContent();

public slots:
	void retranslate();
	void refreshUpdateValues(void);

private slots:
	void setUpdateValues(int hours);
	void setUpdatesEnabled(int checkState);
	void setDistributionEnabled(int checkState);
	void setDisplayAtStartupEnabled(int checkState);
	void setDisplayShowMeteorShowerButton(int checkState);
	void updateStateReceiver(MeteorShowers::UpdateState state);
        void updateCompleteReceiver();
	void restoreDefaults(void);
	void saveSettings(void);
	void updateJSON(void);

private:
        Ui_meteorShowerDialog* ui;
	void setAboutHtml(void);
	void updateGuiFromSettings(void);
	QTimer* updateTimer;

};

#endif // _METEORSHOWERDIALOG_HPP_
