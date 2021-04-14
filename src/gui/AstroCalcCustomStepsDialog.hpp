/*
 * Stellarium
 * Copyright (C) 2020 Alexander Wolf
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

#ifndef ASTROCALCCUSTOMSTEPSDIALOG_HPP
#define ASTROCALCCUSTOMSTEPSDIALOG_HPP

#include "StelDialog.hpp"

#include <QObject>

class Ui_astroCalcCustomStepsDialogForm;

//! @class AstroCalcCustomStepsDialog
class AstroCalcCustomStepsDialog : public StelDialog
{
	Q_OBJECT

public:
	AstroCalcCustomStepsDialog();
	virtual ~AstroCalcCustomStepsDialog() Q_DECL_OVERRIDE;

public slots:
	virtual void retranslate() Q_DECL_OVERRIDE;

	void populateUnitMeasurementsList();
	void saveTimeStep(double value);
	void saveUnitMeasurement(int index);

private:
	QSettings* conf;

protected:
	//! Initialize the dialog widgets and connect the signals/slots.
	virtual void createDialogContent() Q_DECL_OVERRIDE;
	Ui_astroCalcCustomStepsDialogForm *ui;
};

#endif // ASTROCALCCUSTOMSTEPSDIALOG_HPP
