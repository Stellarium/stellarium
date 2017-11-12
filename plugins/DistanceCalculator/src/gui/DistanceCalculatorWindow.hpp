/*
 * Distance Calculator plug-in for Stellarium
 *
 * Copyright (C) 2017 Alexander Wolf
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _DISTANCE_CALCULATOR_WINDOW_HPP_
#define _DISTANCE_CALCULATOR_WINDOW_HPP_

#include "StelDialog.hpp"
#include "SolarSystem.hpp"

#include <QString>

class Ui_distanceCalculatorWindowForm;
class DistanceCalculator;

//! Main window of the Distance Calculator plug-in.
//! @ingroup distanceCalculator
class DistanceCalculatorWindow : public StelDialog
{
	Q_OBJECT

public:
	DistanceCalculatorWindow();
	~DistanceCalculatorWindow();

public slots:
	void retranslate();

protected:
	void createDialogContent();

private:
	Ui_distanceCalculatorWindowForm* ui;
	DistanceCalculator* distanceCalc;
	SolarSystem* solarSystem;
	StelCore* core;

	void setAboutHtml();
	void populateCelestialBodies();

private slots:
	void saveDistanceSettings();
	void resetDistanceSettings();
	void computeDistance();

};


#endif /* _DISTANCE_CALCUALTOR_WINDOW_HPP_ */
