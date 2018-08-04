/*
 * Stellarium
 * 
 * Copyright (C) 2016 Alexander Wolf
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

#ifndef CONFIGUREORBITCOLORSDIALOG_HPP
#define CONFIGUREORBITCOLORSDIALOG_HPP

#include <QObject>
#include "StelDialog.hpp"

class QToolButton;
class Ui_ConfigureOrbitColorsDialogForm;

class ConfigureOrbitColorsDialog : public StelDialog
{
	Q_OBJECT

public:
	ConfigureOrbitColorsDialog();
	virtual ~ConfigureOrbitColorsDialog();

public slots:
        void retranslate();

private slots:
	void askGenericOrbitColor();
	void askMajorPlanetsGroupOrbitColor();
	void askMinorPlanetsGroupOrbitColor();
	void askDwarfPlanetsGroupOrbitColor();
	void askMoonsGroupOrbitColor();
	void askCubewanosGroupOrbitColor();
	void askPlutinosGroupOrbitColor();
	void askSDOGroupOrbitColor();
	void askOCOGroupOrbitColor();
	void askCometsGroupOrbitColor();
	void askSednoidsGroupOrbitColor();
	void askMercuryOrbitColor();
	void askVenusOrbitColor();
	void askEarthOrbitColor();
	void askMarsOrbitColor();
	void askJupiterOrbitColor();
	void askSaturnOrbitColor();
	void askUranusOrbitColor();
	void askNeptuneOrbitColor();
	void setColorStyle();

private:
	void colorButton(QToolButton *toolButton, QString propName);

protected:
        //! Initialize the dialog widgets and connect the signals/slots.
        virtual void createDialogContent();
	Ui_ConfigureOrbitColorsDialogForm *ui;

};

#endif // CONFIGUREORBITCOLORSDIALOG_HPP
