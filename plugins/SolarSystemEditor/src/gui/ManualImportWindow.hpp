/*
 * Solar System editor plug-in for Stellarium
 *
 * Copyright (C) 2010 Bogdan Marinov
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#ifndef _MANUAL_IMPORT_WINDOW_
#define _MANUAL_IMPORT_WINDOW_

#include <QObject>
#include "StelDialog.hpp"

#include "SolarSystemEditor.hpp"

#include <QColor>

class Ui_manualImportWindow;
class QLineEdit;

/*! \brief Window for manual entry of Solar System object properties.
  \author Bogdan Marinov
*/
class ManualImportWindow : public StelDialog
{
	Q_OBJECT
public:
	ManualImportWindow();
	virtual ~ManualImportWindow();

public slots:
	void retranslate();

private slots:
	//TODO: Object type

	void selectColor();
	void parseColorString(QString);

	void toggleCometOrbit(bool);
	void toggleEllipticOrbit(bool);
	void toggleObjectSpecificOrbit(bool);

	void toggleMeanMotionOrPeriod(bool);

	void selectPlanetTextureFile();
	void selectRingTextureFile();
	//TODO: Parse input in the line edits? (Otherwise, leave them read-only.)

private:
	SolarSystemEditor * ssoManager;

	QColor objectColor;

	void setColorButtonColor(QColor newColor);

	void selectTextureFile(QLineEdit * filePathLineEdit);
	//! Check if a file is a valid graphics file with power-of-two texture dimensions.
	//! StelRenderer only guarantees support for dimentions that are powers of 2 (512, 1024, etc.)
	bool verifyTextureFile(QString filePath);

protected:
	virtual void createDialogContent();
	Ui_manualImportWindow * ui;
};

#endif //_MANUAL_IMPORT_WINDOW_
