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

#ifndef OCULARDIALOG_HPP
#define OCULARDIALOG_HPP

#include <QObject>
#include "CCD.hpp"
#include "Ocular.hpp"
#include "PropertyBasedTableModel.hpp"
#include "StelDialog.hpp"
#include "StelStyle.hpp"
#include "Telescope.hpp"
#include "Lens.hpp"
#include "VecMath.hpp"

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

//! @ingroup oculars
class OcularDialog : public StelDialog
{
	Q_OBJECT

public:
	OcularDialog(Oculars* plugin, QList<CCD *>* ccds, QList<Ocular *>* oculars, QList<Telescope *>* telescopes, QList<Lens *>* lenses);
	virtual ~OcularDialog();

public slots:
	void closeWindow();
	void deleteSelectedCCD();
	void deleteSelectedOcular();
	void deleteSelectedTelescope();
	void deleteSelectedLens();
	void insertNewCCD();
	void insertNewOcular();
	void insertNewTelescope();
	void insertNewLens();
	void moveUpSelectedSensor();
	void moveUpSelectedOcular();
	void moveUpSelectedTelescope();
	void moveUpSelectedLens();
	void moveDownSelectedSensor();
	void moveDownSelectedOcular();
	void moveDownSelectedTelescope();
	void moveDownSelectedLens();
	void retranslate();

	// Mini-methods required to immediately update display
	void updateOcular();
	void selectOcular(const QModelIndex);
	void updateLens();
	void selectLens(const QModelIndex);
	void updateCCD();
	void selectCCD(const QModelIndex);
	void updateTelescope();
	void selectTelescope(const QModelIndex);

protected:
	//! Initialize the dialog widgets and connect the signals/slots
	virtual void createDialogContent();
	Ui_ocularDialogForm* ui;

private slots:
	void initAboutText();
	void setLabelsDescriptionText(bool state);
	void updateTelradCustomFOV();	
	void setupTelradFOVspins(Vec4f fov);
	void updateGuiOptions();

private:
	Oculars* plugin;

	QDataWidgetMapper*		ccdMapper;
	QList<CCD *>*			ccds;
	PropertyBasedTableModel*	ccdTableModel;
	QDataWidgetMapper*		ocularMapper;
	QList<Ocular *>*		oculars;
	PropertyBasedTableModel*	ocularTableModel;
	QDataWidgetMapper*		telescopeMapper;
	QList<Telescope *>*		telescopes;
	PropertyBasedTableModel*	telescopeTableModel;
	QDataWidgetMapper*		lensMapper;
	QList<Lens *>*			lenses;
	PropertyBasedTableModel*	lensTableModel;
	QRegExpValidator*		validatorName;
};

#endif // OCULARDIALOG_HPP
