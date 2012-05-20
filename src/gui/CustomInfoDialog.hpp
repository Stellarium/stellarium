/*
 * Stellarium
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
*/

// AW: Methods copied largely from AddRemoveLandscapesDialog

#ifndef _CUSTOMINFODIALOG_HPP_
#define _CUSTOMINFODIALOG_HPP_

#include <QObject>
#include <QSettings>
#include "StelDialog.hpp"
#include "StelGui.hpp"

class Ui_CustomInfoDialogForm;

//! @class CustomInfoDialog
class CustomInfoDialog : public StelDialog
{
    Q_OBJECT

public:
    CustomInfoDialog();
    virtual ~CustomInfoDialog();

public slots:
        void retranslate();
	void setVisible(bool);

protected:
	//! Initialize the dialog widgets and connect the signals/slots.
	virtual void createDialogContent();
	Ui_CustomInfoDialogForm *ui;

private slots:
	bool getNameCustomInfoFlag();
	void setNameCustomInfoFlag(bool flag);

	bool getCatalogNumberCustomInfoFlag();
	void setCatalogNumberCustomInfoFlag(bool flag);

	bool getMagnitudeCustomInfoFlag();
	void setMagnitudeCustomInfoFlag(bool flag);

	bool getRaDecJ2000CustomInfoFlag();
	void setRaDecJ2000CustomInfoFlag(bool flag);

	bool getRaDecOfDateCustomInfoFlag();
	void setRaDecOfDateCustomInfoFlag(bool flag);

	bool getAltAzCustomInfoFlag();
	void setAltAzCustomInfoFlag(bool flag);

	bool getDistanceCustomInfoFlag();
	void setDistanceCustomInfoFlag(bool flag);

	bool getSizeCustomInfoFlag();
	void setSizeCustomInfoFlag(bool flag);

	bool getExtra1CustomInfoFlag();
	void setExtra1CustomInfoFlag(bool flag);

	bool getExtra2CustomInfoFlag();
	void setExtra2CustomInfoFlag(bool flag);

	bool getExtra3CustomInfoFlag();
	void setExtra3CustomInfoFlag(bool flag);

	bool getHourAngleCustomInfoFlag();
	void setHourAngleCustomInfoFlag(bool flag);

	bool getAbsoluteMagnitudeCustomInfoFlag();
	void setAbsoluteMagnitudeCustomInfoFlag(bool flag);

private:
	StelGuiBase* gui;
	QSettings* conf;

};

#endif // _CUSTOMINFODIALOG_HPP_
