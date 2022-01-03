/*
 * Stellarium
 * 
 * Copyright (C) 2011 Georg Zotti (Refraction/Extinction feature)
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


// GZ: Methods copied largely from AddRemoveLandscapesDialog

#ifndef SKYLIGHTDIALOG_HPP
#define SKYLIGHTDIALOG_HPP

#include <QObject>
#include "StelDialog.hpp"
//#include "RefractionExtinction.hpp"
#include "Skylight.hpp"


class Ui_skylightDialogForm;

class SkylightDialog : public StelDialog
{
	Q_OBJECT

public:
	SkylightDialog();
	virtual ~SkylightDialog() Q_DECL_OVERRIDE;

public slots:
	virtual void retranslate() Q_DECL_OVERRIDE;

protected slots:
	// various reset functions for the parametrizable atmosphere model.
	void resetYPreet();
	void resetxPreet();
	void resetyPreet();
	void resetYStel();
	void resetxStel();
	void resetyStel();

	//void resetZYPreet();
	void resetZxPreet();
	void resetZyPreet();
	//void resetZYStel();
	void resetZxStel();
	void resetZyStel();

	void resetPreet(); // reset all xyYABCDEF parameters
	void resetStel();  // reset all xyYABCDEF parameters

	void setIncrements(int idx);
	//void setTfromK(double k);

protected:
        //! Initialize the dialog widgets and connect the signals/slots.
	virtual void createDialogContent() Q_DECL_OVERRIDE;
	Ui_skylightDialogForm *ui;

private:
	Skylight *skylight;
};

#endif // SKYLIGHTDIALOG_HPP
