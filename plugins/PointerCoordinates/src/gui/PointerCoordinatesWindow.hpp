/*
 * Pointer Coordinates plug-in for Stellarium
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

#ifndef POINTERCOORDINATESWINDOW_HPP
#define POINTERCOORDINATESWINDOW_HPP

#include "StelDialog.hpp"

#include <QString>
#include <QDoubleSpinBox>

class Ui_pointerCoordinatesWindowForm;
class PointerCoordinates;

//! Main window of the Pointer Coordinates plug-in.
//! @ingroup pointerCoordinates
class PointerCoordinatesWindow : public StelDialog
{
	Q_OBJECT

public:
	PointerCoordinatesWindow();
	~PointerCoordinatesWindow();

public slots:
	void retranslate();

protected:
	void createDialogContent();

private:
	Ui_pointerCoordinatesWindowForm* ui;
	PointerCoordinates* coord;

	void setAboutHtml();
	void populateValues();
	void setCustomCoordinatesAccess(QString place);

private slots:
	void saveCoordinatesSettings();
	void resetCoordinatesSettings();

	void populateCoordinatesPlacesList();
	void populateCoordinateSystemsList();
	void setCoordinatesPlace(int placeID);
	void setCoordinateSystem(int csID);
	void setCustomCoordinatesPlace();
};


#endif /* POINTERCOORDINATESWINDOW_HPP */
