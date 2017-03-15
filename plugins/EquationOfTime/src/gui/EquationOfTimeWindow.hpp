/*
 * Equation of Time plug-in for Stellarium
 *
 * Copyright (C) 2014 Alexander Wolf
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

#ifndef _EQUATION_OF_TIME_WINDOW_HPP_
#define _EQUATION_OF_TIME_WINDOW_HPP_

#include "StelDialog.hpp"

#include <QString>
#include <QDoubleSpinBox>

class Ui_equationOfTimeWindowForm;
class EquationOfTime;

//! Main window of the Equation of Time plug-in.
//! @ingroup equationOfTime
class EquationOfTimeWindow : public StelDialog
{
	Q_OBJECT

public:
	EquationOfTimeWindow();
	~EquationOfTimeWindow();

public slots:
	void retranslate();

protected:
	void createDialogContent();

private:
	Ui_equationOfTimeWindowForm* ui;
	EquationOfTime* eq;

	void updateAboutText();		

private slots:
	void saveEquationOfTimeSettings();
	void resetEquationOfTimeSettings();


};


#endif /* _EQUATION_OF_TIME_WINDOW_HPP_ */
