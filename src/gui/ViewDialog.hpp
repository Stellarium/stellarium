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

#include <QObject>
#include "StelDialog.hpp"

class Ui_viewDialogForm;
class QListWidgetItem;

class AddRemoveLandscapesDialog;
class AtmosphereDialog;

class ViewDialog : public StelDialog
{
Q_OBJECT
public:
	ViewDialog();
	virtual ~ViewDialog();
	//! Notify that the application style changed
	void styleChanged();

public slots:
	void retranslate();
	void updateIconsColor();

protected:
	Ui_viewDialogForm* ui;
	//! Initialize the dialog widgets and connect the signals/slots
	virtual void createDialogContent();
private slots:
	void populateLists();
	void skyCultureChanged(const QString& cultureName);
	void projectionChanged(const QString& projectionName);
	void landscapeChanged(QListWidgetItem* item);
	void setZhrFromControls();
	void updateZhrControls(int zhr);
	void updateZhrDescription(int zhr);
	void planetsLabelsValueChanged(int);
	void nebulasLabelsValueChanged(int);
	void starsLabelsValueChanged(int);
	void setCurrentLandscapeAsDefault(void);
	void setCurrentCultureAsDefault(void);
	//! Update the widget to make sure it is synchrone if a value was changed programmatically
	//! This function should be called repeatidly with e.g. a timer
	void updateFromProgram();

	void showAddRemoveLandscapesDialog();
        void showAtmosphereDialog();

	void populateSkyLayersList();
	void skyLayersSelectionChanged(const QString&);
	void skyLayersEnabledChanged(int);

	void changePage(QListWidgetItem *current, QListWidgetItem *previous);
private:
	void updateSkyCultureText();

	AddRemoveLandscapesDialog * addRemoveLandscapesDialog;
        AtmosphereDialog * atmosphereDialog;
};

#endif // _VIEWDIALOG_HPP_
