/*
 * FOV plug-in for Stellarium
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

#ifndef FOVWINDOW_HPP
#define FOVWINDOW_HPP

#include "StelDialog.hpp"

#include <QString>
#include <QDoubleSpinBox>

class Ui_fovWindowForm;
class FOV;

//! Main window of the FOV plug-in.
//! @ingroup fieldOfView
class FOVWindow : public StelDialog
{
	Q_OBJECT

public:
	FOVWindow();
	~FOVWindow();

public slots:
	void retranslate();

protected:
	void createDialogContent();

private:
	Ui_fovWindowForm* ui;
	FOV* fov;

	void setAboutHtml();
	void populateFOV();

private slots:
	void saveFOVSettings();
	void resetFOVSettings();
	void updateFOV0(double value);
	void updateFOV1(double value);
	void updateFOV2(double value);
	void updateFOV3(double value);
	void updateFOV4(double value);
	void updateFOV5(double value);
	void updateFOV6(double value);
	void updateFOV7(double value);
	void updateFOV8(double value);
	void updateFOV9(double value);
};


#endif /* FOVWINDOW_HPP */
