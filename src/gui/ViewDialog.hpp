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

#ifndef _VIEWDIALOG_HPP_
#define _VIEWDIALOG_HPP_

#include "StelDialog.hpp"

#include <QObject>

class Ui_viewDialogForm;
class QListWidgetItem;

class AddRemoveLandscapesDialog;
class AtmosphereDialog;
class GreatRedSpotDialog;

class ViewDialog : public StelDialog
{
Q_OBJECT
public:
	ViewDialog(QObject* parent);
	virtual ~ViewDialog();
	//! Notify that the application style changed
	void styleChanged();

public slots:
	void retranslate();

protected:
	Ui_viewDialogForm* ui;
	//! Initialize the dialog widgets and connect the signals/slots
	virtual void createDialogContent();
private slots:
	void populateLists();
	void skyCultureChanged(const QString& cultureName);
	void projectionChanged(const QString& projectionName);
	void viewportVerticalShiftChanged(const int shift);
	void landscapeChanged(QListWidgetItem* item);
	void setZHR(int zhr);
	void updateZhrDescription();
	void planetsLabelsValueChanged(int);
	void nebulasLabelsValueChanged(int);
	void nebulasMarkersValueChanged(int);
	void setBortleScaleToolTip(int Bindex);
	void starsLabelsValueChanged(int);
	void setCurrentLandscapeAsDefault(void);
	void setCurrentCultureAsDefault(void);
	void setFlagLandscapeUseMinimalBrightness(bool b);
	void setFlagCustomGrsSettings(bool b);
	//! Update the widget to make sure it is synchrone if a value was changed programmatically
	//! This function should be called repeatidly with e.g. a timer
	void updateFromProgram();

	void showAddRemoveLandscapesDialog();
        void showAtmosphereDialog();
	void showGreatRedSpotDialog();

	void populateLightPollution();
	void populateLandscapeMinimalBrightness();

	// WHAT IS THE SKY LAYER? hidden, under development?
	void populateSkyLayersList();
	void skyLayersSelectionChanged(const QString&);
	void skyLayersEnabledChanged(int);

	void setSelectedCatalogsFromCheckBoxes();
	void setSelectedTypesFromCheckBoxes();

	void changePage(QListWidgetItem *current, QListWidgetItem *previous);
private:
	//! convenience method to link a checkbox to a StelAction.
	void connectCheckBox(class QCheckBox* checkBox, const QString& actionId);
	void connectGroupBox(class QGroupBox* groupBox, const QString& actionId);
	void updateSkyCultureText();
	//! Make sure that no tabs icons are outside of the viewport.
	//! @todo Limit the width to the width of the screen *available to the window*.
	void updateTabBarListWidgetWidth();

	void updateSelectedCatalogsCheckBoxes();
	void updateSelectedTypesCheckBoxes();

	AddRemoveLandscapesDialog * addRemoveLandscapesDialog;
	AtmosphereDialog * atmosphereDialog;
	GreatRedSpotDialog * greatRedSpotDialog;
};

#endif // _VIEWDIALOG_HPP_
