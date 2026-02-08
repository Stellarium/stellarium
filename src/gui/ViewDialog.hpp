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
#include <QTimer>
#include <QHash>
#include "StelObject.hpp"

class Ui_viewDialogForm;
class QListWidgetItem;
class QTreeWidgetItem;
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
	void configureSkyCultureCheckboxes();
	void updateSkyCultureInfoStyleFromCheckboxes();
	void updateSkyCultureScreenStyleFromCheckboxes();
	void populateCulturalCombo(QComboBox *combo, StelObject::CulturalDisplayStyle style);
	void setZodiacLabelStyle(int index);
	void setLunarSystemLabelStyle(int index);
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
	void updateSkyCultureTimeValue(int year);
	void changeMinTime(int minYear);
	void changeMaxTime(int maxYear);
	void updateSkyCultureTimeRange(int minYear, int maxYear);
	void filterSkyCultures();
	void initiateSkyCultureMapRotation();

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

	void clearHips();
	void updateHips();
	void updateHipsText();
	void filterSurveys();
	void hipsListItemChanged(QTreeWidgetItem* item);
	void populateHipsGroups();
	void toggleHipsDialog();

	void populateOrbitsControls(bool flag);
	void populateTrailsControls(bool flag);
	void populateNomenclatureControls(bool flag);

	void setDisplayFormatForSpins(bool flagDecimalDegrees);

private:
	void connectGroupBox(class QGroupBox* groupBox, const QString& actionId);
	void updateSkyCultureText();
	void initSkyCultureTime();
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

	QTimer hipsUpdateTimer;
	struct PlanetSurveyPack
	{
		QTreeWidgetItem* planetItem = nullptr;
		QHash<QString/*group*/, QTreeWidgetItem* /*groupItem*/> groupsMap;
	};
	QHash<QString/*planet English name*/, PlanetSurveyPack> planetarySurveys;
	QHash<QString/*survey URL*/, QTreeWidgetItem*> surveysInTheList;
	QString selectedSurveyType;

	// list of special characters and their 'normal' counterpart for filtering (skycultures / the culturesListWidget)ĀA
	// (when a new special character is added to 'specialCharString', 'normalCharList' must also be expanded)
	QString specialCharString = "ŠŒŽšœžŸ¥µÀÁÂÃĀÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖØÙÚÛÜÝßàáâãāäåæçèéêëìíîïðñòóôõöøùúûüýÿ";
	QStringList normalCharList = QStringList({"S", "OE", "Z", "s", "oe", "z", "Y", "Y", "u", "A", "A", "A", "A", "A", "A", "A",
										"AE", "C", "E", "E", "E", "E", "I", "I", "I", "I", "D", "N", "O", "O", "O", "O", "O", "O",
										"U", "U", "U", "U", "Y", "s", "a", "a", "a", "a", "a", "a", "a", "ae", "c", "e", "e", "e", "e",
										"i", "i", "i", "i","o", "n", "o", "o", "o", "o", "o", "o", "u", "u", "u", "u", "y", "y"});
};

#endif // _VIEWDIALOG_HPP
