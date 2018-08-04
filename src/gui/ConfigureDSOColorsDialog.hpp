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

#ifndef CONFIGUREDSOCOLORSDIALOG_HPP
#define CONFIGUREDSOCOLORSDIALOG_HPP

#include <QObject>
#include "StelDialog.hpp"

class QToolButton;
class Ui_ConfigureDSOColorsDialogForm;

class ConfigureDSOColorsDialog : public StelDialog
{
	Q_OBJECT

public:
	ConfigureDSOColorsDialog();
	virtual ~ConfigureDSOColorsDialog();

public slots:
        void retranslate();

private slots:
	// DSO markers and labels colors
	void askDSOLabelsColor();
	void askDSOMarkersColor();
	void askDSOGalaxiesColor();
	void askDSOActiveGalaxiesColor();
	void askDSORadioGalaxiesColor();
	void askDSOInteractingGalaxiesColor();
	void askDSOQuasarsColor();
	void askDSOPossibleQuasarsColor();
	void askDSOStarClustersColor();
	void askDSOOpenStarClustersColor();
	void askDSOGlobularStarClustersColor();
	void askDSOStellarAssociationsColor();
	void askDSOStarCloudsColor();
	void askDSOStarsColor();
	void askDSOSymbioticStarsColor();
	void askDSOEmissionLineStarsColor();
	void askDSONebulaeColor();
	void askDSOPlanetaryNebulaeColor();
	void askDSODarkNebulaeColor();
	void askDSOReflectionNebulaeColor();
	void askDSOBipolarNebulaeColor();
	void askDSOEmissionNebulaeColor();
	void askDSONebulosityClustersColor();
	void askDSOPossiblePlanetaryNebulaeColor();
	void askDSOProtoplanetaryNebulaeColor();
	void askDSOHydrogenRegionsColor();
	void askDSOInterstellarMatterColor();
	void askDSOEmissionObjectsColor();
	void askDSOMolecularCloudsColor();
	void askDSOBLLacObjectsColor();
	void askDSOBlazarsColor();
	void askDSOYoungStellarObjectsColor();
	void askDSOSupernovaRemnantsColor();
	void askDSOSupernovaCandidatesColor();
	void askDSOSupernovaRemnantCandidatesColor();
	void askDSOGalaxyClustersColor();

private:
	void colorButton(QToolButton *toolButton, QString propName);

protected:
        //! Initialize the dialog widgets and connect the signals/slots.
        virtual void createDialogContent();
	Ui_ConfigureDSOColorsDialogForm *ui;

};

#endif // CONFIGUREDSOCOLORSDIALOG_HPP
