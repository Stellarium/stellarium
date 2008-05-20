/*
 * Stellarium
 * Copyright (C) 2008 Guillaume Chereau
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
 
#ifndef _LOCATIONDIALOG_HPP_
#define _LOCATIONDIALOG_HPP_

#include <QObject>
#include "StelDialog.hpp"

class Ui_locationDialogForm;

class LocationDialog : public StelDialog
{
	Q_OBJECT;
public:
	LocationDialog();
	virtual ~LocationDialog();
	void languageChanged();
public slots:
	//! Called when the mapView emit the positionSelected signal
	void selectPosition(double longitude, double latitude, int altitude, QString city);
	//! Called when the mapView emit the positionHighlighted signal
	void highlightPosition(double longitude, double latitude, int altitude, QString city);
	//! Called when the user direclty change the location from the spinbox
	void spinBoxChanged();
protected:
	//! Initialize the dialog widgets and connect the signals/slots
	virtual void createDialogContent();

	Ui_locationDialogForm* ui;
};

#endif // _LOCATIONDIALOG_HPP_
