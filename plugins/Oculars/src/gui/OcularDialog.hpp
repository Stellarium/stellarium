/*
 * Copyright (C) 2009 Timothy Reaves
 * Copyright (C) 2011 Bogdan Marinov
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

#ifndef _OCULARDIALOG_HPP_
#define _OCULARDIALOG_HPP_

#include <QObject>
#include "CCD.hpp"
#include "Ocular.hpp"
#include "PropertyBasedTableModel.hpp"
#include "StelDialog.hpp"
#include "StelStyle.hpp"
#include "Telescope.hpp"
#include "Barlow.hpp"

class Ui_ocularDialogForm;

QT_BEGIN_NAMESPACE
class QDataWidgetMapper;
class QDoubleValidator;
class QIntValidator;
class QRegExpValidator;
class QModelIndex;
class QStandardItemModel;
QT_END_NAMESPACE

class Oculars;

class OcularDialog : public StelDialog
{
	Q_OBJECT

public:
    OcularDialog(Oculars* plugin, QList<CCD *>* ccds, QList<Ocular *>* oculars, QList<Telescope *>* telescopes, QList<Barlow *>* barlows);
	virtual ~OcularDialog();
	//! Notify that the application style changed
	void styleChanged();
	void updateStyle();

public slots:
	void closeWindow();
	void deleteSelectedCCD();
	void deleteSelectedOcular();
	void deleteSelectedTelescope();
	void deleteSelectedBarlow();
	void insertNewCCD();
	void insertNewOcular();
	void insertNewTelescope();
	void insertNewBarlow();
	void moveUpSelectedSensor();
	void moveUpSelectedOcular();
	void moveUpSelectedTelescope();
	void moveUpSelectedBarlow();
	void moveDownSelectedSensor();
	void moveDownSelectedOcular();
	void moveDownSelectedTelescope();
	void moveDownSelectedBarlow();
	void retranslate();

signals:
	void requireSelectionChanged(bool state);
	void scaleImageCircleChanged(bool state);

protected:
	//! Initialize the dialog widgets and connect the signals/slots
	virtual void createDialogContent();
	Ui_ocularDialogForm* ui;

private slots:
	void keyBindingTogglePluginChanged(const QString& newString);
	void keyBindingPopupNavigatorConfigChanged(const QString& newString);
	void initAboutText();
	void requireSelectionStateChanged(int state);
	void scaleImageCircleStateChanged(int state);

private:
	Oculars* plugin;

	QDataWidgetMapper*			ccdMapper;
	QList<CCD *>*					ccds;
	PropertyBasedTableModel*	ccdTableModel;
	QDataWidgetMapper*			ocularMapper;
	QList<Ocular *>*				oculars;
	PropertyBasedTableModel*	ocularTableModel;
	QDataWidgetMapper*			telescopeMapper;
	QList<Telescope *>*			telescopes;
	PropertyBasedTableModel*	telescopeTableModel;
	QDataWidgetMapper*			barlowMapper;
	QList<Barlow *>*			barlows;
	PropertyBasedTableModel*	barlowTableModel;
	QDoubleValidator*				validatorOcularAFOV;
	QDoubleValidator*				validatorOcularEFL;
	QDoubleValidator*				validatorTelescopeDiameter;
	QDoubleValidator*				validatorTelescopeFL;
	QDoubleValidator*				validatorBarlowMultipler;
	QRegExpValidator*				validatorName;
	QIntValidator*					validatorPositiveInt;
	QDoubleValidator*				validatorPositiveDouble;
};

#endif // _OCULARDIALOG_HPP_
