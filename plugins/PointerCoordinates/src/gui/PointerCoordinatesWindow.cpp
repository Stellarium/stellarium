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

#include "PointerCoordinates.hpp"
#include "PointerCoordinatesWindow.hpp"
#include "ui_pointerCoordinatesWindow.h"

#include "StelApp.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModule.hpp"
#include "StelModuleMgr.hpp"

PointerCoordinatesWindow::PointerCoordinatesWindow()
	: coord(NULL)
{
	ui = new Ui_pointerCoordinatesWindowForm();
	dialogName = "PointerCoordinates";
}

PointerCoordinatesWindow::~PointerCoordinatesWindow()
{
	delete ui;
}

void PointerCoordinatesWindow::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		updateAboutText();
		populateCoordinatesPlacesList();
		populateCoordinateSystemsList();
	}
}

void PointerCoordinatesWindow::createDialogContent()
{
	coord = GETSTELMODULE(PointerCoordinates);
	ui->setupUi(dialog);

	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->TitleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));

	populateValues();

	connect(ui->checkBoxEnableAtStartup, SIGNAL(clicked(bool)), coord, SLOT(setFlagEnableAtStartup(bool)));
	connect(ui->spinBoxFontSize, SIGNAL(valueChanged(int)), coord, SLOT(setFontSize(int)));
	connect(ui->checkBoxShowButton, SIGNAL(clicked(bool)), coord, SLOT(setFlagShowCoordinatesButton(bool)));

	// Place of the string with coordinates
	populateCoordinatesPlacesList();
	QString currentPlaceKey = coord->getCurrentCoordinatesPlaceKey();
	int idx = ui->placeComboBox->findData(currentPlaceKey, Qt::UserRole, Qt::MatchCaseSensitive);
	if (idx==-1)
	{
		// Use TopRight as default
		idx = ui->placeComboBox->findData(QVariant("TopRight"), Qt::UserRole, Qt::MatchCaseSensitive);
	}
	ui->placeComboBox->setCurrentIndex(idx);
	setCustomCoordinatesAccess(currentPlaceKey);
	connect(ui->placeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setCoordinatesPlace(int)));

	populateCoordinateSystemsList();
	idx = ui->coordinateSystemComboBox->findData(coord->getCurrentCoordinateSystemKey(), Qt::UserRole, Qt::MatchCaseSensitive);
	if (idx==-1)
	{
		// Use RaDecJ2000 as default
		idx = ui->coordinateSystemComboBox->findData(QVariant("RaDecJ2000"), Qt::UserRole, Qt::MatchCaseSensitive);
	}
	ui->coordinateSystemComboBox->setCurrentIndex(idx);
	connect(ui->coordinateSystemComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setCoordinateSystem(int)));

	connect(ui->spinBoxX, SIGNAL(valueChanged(int)), this, SLOT(setCustomCoordinatesPlace()));
	connect(ui->spinBoxY, SIGNAL(valueChanged(int)), this, SLOT(setCustomCoordinatesPlace()));

	connect(ui->pushButtonSave, SIGNAL(clicked()), this, SLOT(saveCoordinatesSettings()));
	connect(ui->pushButtonReset, SIGNAL(clicked()), this, SLOT(resetCoordinatesSettings()));

	updateAboutText();
}

void PointerCoordinatesWindow::populateValues()
{
	ui->checkBoxEnableAtStartup->setChecked(coord->getFlagEnableAtStartup());
	ui->spinBoxFontSize->setValue(coord->getFontSize());
	ui->checkBoxShowButton->setChecked(coord->getFlagShowCoordinatesButton());
	QPair<int, int> cc = coord->getCustomCoordinatesPlace();
	ui->spinBoxX->setValue(cc.first);
	ui->spinBoxY->setValue(cc.second);
}

void PointerCoordinatesWindow::updateAboutText()
{
	ui->labelTitle->setText(q_("Pointer Coordinates plug-in"));
	QString version = QString(q_("Version %1")).arg(POINTERCOORDINATES_PLUGIN_VERSION);
	ui->labelVersion->setText(version);
}

void PointerCoordinatesWindow::saveCoordinatesSettings()
{
	coord->saveConfiguration();
}

void PointerCoordinatesWindow::resetCoordinatesSettings()
{
	coord->restoreDefaultConfiguration();
	populateValues();
}

void PointerCoordinatesWindow::populateCoordinatesPlacesList()
{
	Q_ASSERT(ui->placeComboBox);

	QComboBox* places = ui->placeComboBox;

	//Save the current selection to be restored later
	places->blockSignals(true);
	int index = places->currentIndex();
	QVariant selectedPlaceId = places->itemData(index);	
	places->clear();
	//For each algorithm, display the localized name and store the key as user
	//data. Unfortunately, there's no other way to do this than with a cycle.
	places->addItem(q_("The top center of the screen"), "TopCenter");
	places->addItem(q_("In center of the top right half of the screen"), "TopRight");
	places->addItem(q_("The right bottom corner of the screen"), "RightBottomCorner");
	places->addItem(q_("Custom position"), "Custom");

	//Restore the selection
	index = places->findData(selectedPlaceId, Qt::UserRole, Qt::MatchCaseSensitive);
	places->setCurrentIndex(index);
	places->blockSignals(false);	
}

void PointerCoordinatesWindow::populateCoordinateSystemsList()
{
	Q_ASSERT(ui->coordinateSystemComboBox);

	QComboBox* csys = ui->coordinateSystemComboBox;

	//Save the current selection to be restored later
	csys->blockSignals(true);
	int index = csys->currentIndex();
	QVariant selectedSystemId = csys->itemData(index);
	csys->clear();
	//For each algorithm, display the localized name and store the key as user
	//data. Unfortunately, there's no other way to do this than with a cycle.
	csys->addItem(q_("Right ascension/Declination (J2000.0)"), "RaDecJ2000");
	csys->addItem(q_("Right ascension/Declination"), "RaDec");
	csys->addItem(q_("Hour angle/Declination"), "HourAngle");
	csys->addItem(q_("Ecliptic Longitude/Latitude"), "Ecliptic");
	csys->addItem(q_("Ecliptic Longitude/Latitude (J2000.0)"), "EclipticJ2000");
	csys->addItem(q_("Altitude/Azimuth"), "AltAzi");
	csys->addItem(q_("Galactic Longitude/Latitude"), "Galactic");

	//Restore the selection
	index = csys->findData(selectedSystemId, Qt::UserRole, Qt::MatchCaseSensitive);
	csys->setCurrentIndex(index);
	csys->blockSignals(false);
}

void PointerCoordinatesWindow::setCoordinatesPlace(int placeID)
{
	QString currentPlaceID = ui->placeComboBox->itemData(placeID).toString();
	coord->setCurrentCoordinatesPlaceKey(currentPlaceID);
	setCustomCoordinatesAccess(currentPlaceID);
}

void PointerCoordinatesWindow::setCoordinateSystem(int csID)
{
	QString currentCsId = ui->coordinateSystemComboBox->itemData(csID).toString();
	coord->setCurrentCoordinateSystemKey(currentCsId);
}

void PointerCoordinatesWindow::setCustomCoordinatesPlace()
{
	coord->setCustomCoordinatesPlace(ui->spinBoxX->value(), ui->spinBoxY->value());
}

void PointerCoordinatesWindow::setCustomCoordinatesAccess(QString place)
{
	if (place.contains("Custom"))
	{
		ui->labelCustomCoords->setText(q_("Coordinates of custom position:"));
		ui->spinBoxX->setVisible(true);
		ui->spinBoxY->setVisible(true);
	}
	else
	{
		ui->labelCustomCoords->setText("");
		ui->spinBoxX->setVisible(false);
		ui->spinBoxY->setVisible(false);
	}
}
