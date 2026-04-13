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
#include <QSettings>

class Ui_astroCalcCustomStepsDialogForm;

//! @class AstroCalcCustomStepsDialog
class AstroCalcCustomStepsDialog : public StelDialog
{
	Q_OBJECT

public:
	AstroCalcCustomStepsDialog();
	~AstroCalcCustomStepsDialog() override;

public slots:
	void retranslate() override;

	void populateUnitMeasurementsList();
	void saveTimeStep(double value);
	void saveUnitMeasurement(int index);
	void saveSunAltitude(double alt);
	void saveSunAltitudeCrossing(int index);
	void saveOppositionPlanet(int index);

	//! Show or hide the Sun-at-altitude / Opposition group boxes depending on
	//! which time step is currently selected in the main ephemeris combo box.
	//! Call this from AstroCalcDialog whenever the step selection changes, and
	//! also just before making the dialog visible.
	void setActiveTimeStep(int stepId);

private:
	QSettings* conf;

protected:
	//! Initialize the dialog widgets and connect the signals/slots.
	void createDialogContent() override;
	Ui_astroCalcCustomStepsDialogForm *ui;
};

#endif // ASTROCALCCUSTOMSTEPSDIALOG_HPP
