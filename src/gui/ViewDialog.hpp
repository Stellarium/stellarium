/*
 * Stellarium
 * Copyright (C) 2008 Fabien Chereau
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

#ifndef VIEWDIALOG_HPP
#define VIEWDIALOG_HPP

#include "StelDialog.hpp"

#include <QObject>

class Ui_viewDialogForm;
class QListWidgetItem;
class QToolButton;

class AddRemoveLandscapesDialog;
class AtmosphereDialog;
class SkylightDialog;
class TonemappingDialog;
class GreatRedSpotDialog;
class ConfigureDSOColorsDialog;
class ConfigureOrbitColorsDialog;

class ViewDialog : public StelDialog
{
Q_OBJECT
public:
	ViewDialog(QObject* parent);
	~ViewDialog() override;

public slots:
	//! Apply application style change
	void styleChanged(const QString &style) override;
	void retranslate() override;

protected:
	Ui_viewDialogForm* ui;
	//! Initialize the dialog widgets and connect the signals/slots
	void createDialogContent() override;
	bool eventFilter(QObject* object, QEvent* event) override;
private slots:
	void populateLists();
	void populateToolTips();
	void skyCultureChanged();
	void changeProjection(const QString& projectionNameI18n);
	void projectionChanged();
	void landscapeChanged(QString id,QString name);
	void updateZhrDescription(int zhr);
	void setCurrentLandscapeAsDefault(void);
	void setCurrentCultureAsDefault(void);
	void updateDefaultSkyCulture();
	void updateDefaultLandscape();

	void showAddRemoveLandscapesDialog();
	// GZ I make this public to have it on a hotkey...
public slots:
	void showAtmosphereDialog();
	void showSkylightDialog();
	void showTonemappingDialog();
	void showGreatRedSpotDialog();
	void showConfigureDSOColorsDialog();
	void showConfigureOrbitColorsDialog();

private slots:
	void populatePlanetMagnitudeAlgorithmsList();
	void populatePlanetMagnitudeAlgorithmDescription();
	void setPlanetMagnitudeAlgorithm(int algorithmID);

	void setSelectedCardinalCheckBoxes();

	void setSelectedCatalogsFromCheckBoxes();
	void setSelectedTypesFromCheckBoxes();

	void changePage(QListWidgetItem *current, QListWidgetItem *previous);

	void updateSelectedCatalogsCheckBoxes();
	void updateSelectedTypesCheckBoxes();

	void updateHips();
	void hipsListItemChanged(QListWidgetItem* item);
	void populateHipsGroups();

	void populateOrbitsControls(bool flag);
	void populateTrailsControls(bool flag);
	void populateNomenclatureControls(bool flag);

	void setDisplayFormatForSpins(bool flagDecimalDegrees);

private:
	void connectGroupBox(class QGroupBox* groupBox, const QString& actionId);
	void updateSkyCultureText();
	//! Make sure that no tabs icons are outside of the viewport.
	//! @todo Limit the width to the width of the screen *available to the window*.
	void updateTabBarListWidgetWidth();

	AddRemoveLandscapesDialog * addRemoveLandscapesDialog;
	AtmosphereDialog * atmosphereDialog;
	SkylightDialog * skylightDialog;
	TonemappingDialog * tonemappingDialog;
	GreatRedSpotDialog * greatRedSpotDialog;
	ConfigureDSOColorsDialog * configureDSOColorsDialog;
	ConfigureOrbitColorsDialog * configureOrbitColorsDialog;
};

#endif // _VIEWDIALOG_HPP
