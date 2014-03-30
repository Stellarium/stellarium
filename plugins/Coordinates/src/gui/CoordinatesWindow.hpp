/*
 * Coordinates plug-in for Stellarium
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

#ifndef _COORDINATES_WINDOW_HPP_
#define _COORDINATES_WINDOW_HPP_

#include "StelDialog.hpp"

#include <QString>
#include <QDoubleSpinBox>

class Ui_coordinatesWindowForm;
class Coordinates;

//! Main window of the Coordinates plug-in.
class CoordinatesWindow : public StelDialog
{
	Q_OBJECT

public:
	CoordinatesWindow();
	~CoordinatesWindow();

public slots:
	void retranslate();

protected:
	void createDialogContent();

private:
	Ui_coordinatesWindowForm* ui;
	Coordinates* coord;

	void updateAboutText();		

private slots:
	void saveCoordinatesSettings();
	void resetCoordinatesSettings();


};


#endif /* _COORDINATES_WINDOW_HPP_ */
