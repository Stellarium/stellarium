/*
 * Stellarium Exoplanets Plug-in GUI
 * 
 * Copyright (C) 2012 Alexander Wolf
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
 
#ifndef _EXOPLANETSDIALOG_HPP_
#define _EXOPLANETSDIALOG_HPP_

#include <QObject>
#include "StelDialog.hpp"
#include "Exoplanets.hpp"

class Ui_exoplanetsDialog;
class QTimer;

class ExoplanetsDialog : public StelDialog
{
	Q_OBJECT

public:
	ExoplanetsDialog();
	~ExoplanetsDialog();

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
	void setTimelineEnabled(int checkState);
	void updateStateReceiver(Exoplanets::UpdateState state);
        void updateCompleteReceiver();
	void restoreDefaults(void);
	void saveSettings(void);
	void updateJSON(void);

private:
        Ui_exoplanetsDialog* ui;
	void setAboutHtml(void);
	void updateGuiFromSettings(void);
	QTimer* updateTimer;

};

#endif // _EXOPLANETSDIALOG_HPP_
