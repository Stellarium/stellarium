/*
 * Stellarium
 *
 * Copyright (C) 2022 Georg Zotti
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

#ifndef SKYLIGHTDIALOG_HPP
#define SKYLIGHTDIALOG_HPP

#include <QObject>
#include "StelDialog.hpp"

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
	//! Reset function for the parametrizable Preetham atmosphere model.
	//! This resets the parameters for the CIE Y distribution to the values found in Preetham's 1999 paper.
	void resetYPreet();
	//! Reset function for the parametrizable Preetham atmosphere model.
	//! This resets the parameters for the CIE x distribution to the values found in Preetham's 1999 paper.
	void resetxPreet();
	//! Reset function for the parametrizable Preetham atmosphere model.
	//! This resets the parameters for the CIE y distribution to the values found in Preetham's 1999 paper.
	void resetyPreet();
	//! Reset function for the parametrizable Preetham atmosphere model.
	//! This resets the parameters for the CIE Y distribution to Stellarium's default values.
	void resetYStel();
	//! Reset function for the parametrizable Preetham atmosphere model.
	//! This resets the parameters for the CIE x distribution to Stellarium's default values.
	void resetxStel();
	//! Reset function for the parametrizable Preetham atmosphere model.
	//! This resets the parameters for the CIE y distribution to Stellarium's default values.
	void resetyStel();

	//! Reset function for the parametrizable Preetham atmosphere model.
	//! This resets the parameters for the CIE x zenith color to the values found in Preetham's 1999 paper.
	void resetZxPreet();
	//! Reset function for the parametrizable Preetham atmosphere model.
	//! This resets the parameters for the CIE y zenith color to the values found in Preetham's 1999 paper.
	void resetZyPreet();
	//! Reset function for the parametrizable Preetham atmosphere model.
	//! This resets the parameters for the CIE x zenith color to Stellarium's default values.
	void resetZxStel();
	//! Reset function for the parametrizable Preetham atmosphere model.
	//! This resets the parameters for the CIE y zenith color to Stellarium's default values.
	void resetZyStel();

	void resetPreet(); //! reset all xyYABCDEF parameters to the values found in Preetham's 1999 paper.
	void resetStel();  //! reset all xyYABCDEF parameters to Stellarium's default values.

	//! changes increments for all fine-tuning spinners
	void setIncrements(int idx);

protected:
        //! Initialize the dialog widgets and connect the signals/slots.
	virtual void createDialogContent() Q_DECL_OVERRIDE;
	Ui_skylightDialogForm *ui;
};

#endif // SKYLIGHTDIALOG_HPP
