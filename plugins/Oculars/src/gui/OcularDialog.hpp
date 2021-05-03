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

#pragma once

#include "CCD.hpp"
#include "Lens.hpp"
#include "Ocular.hpp"
#include "PropertyBasedTableModel.hpp"
#include "StelDialog.hpp"
#include "StelStyle.hpp"
#include "Telescope.hpp"
#include "VecMath.hpp"

#include <QDataWidgetMapper>
#include <QObject>
#include <QtGlobal>

class Ui_ocularDialogForm;

class QRegExpValidator;
class Oculars;

//! @ingroup oculars
class OcularDialog : public StelDialog
{
   Q_OBJECT
#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
   Q_DISABLE_COPY_MOVE(OcularDialog)
#endif

public:
   OcularDialog(Oculars *            plugin,
                QList<CCD *> *       ccds,
                QList<Ocular *> *    oculars,
                QList<Telescope *> * telescopes,
                QList<Lens *> *      lenses);
   ~OcularDialog() override;

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
   void retranslate() override;

   // Mini-methods required to immediately update display
   void updateOcular();
   void selectOcular(QModelIndex newIndex) const;
   void updateLens();
   void selectLens(QModelIndex newIndex) const;
   void updateCCD();
   void selectCCD(QModelIndex newIndex) const;
   void updateTelescope();
   void selectTelescope(QModelIndex newIndex) const;

protected:
   //! Initialize the dialog widgets and connect the signals/slots
   void createDialogContent() override;

private slots:
   void initAboutText();
   void setLabelsDescriptionText(bool state);
   void updateTelradCustomFOV(double newValue);
   void setupTelradFOVspins(Vec4d fov);
   void updateGuiOptions();

private:
   Ui_ocularDialogForm *      ui;
   Oculars *                  plugin;

   QDataWidgetMapper          ccdMapper{};
   const QList<CCD *> *       ccds;
   PropertyBasedTableModel *  ccdTableModel;
   QDataWidgetMapper          ocularMapper{};
   const QList<Ocular *> *    oculars;
   PropertyBasedTableModel *  ocularTableModel;
   QDataWidgetMapper          telescopeMapper{};
   const QList<Telescope *> * telescopes;
   PropertyBasedTableModel *  telescopeTableModel;
   QDataWidgetMapper          lensMapper{};
   const QList<Lens *> *      lenses;
   PropertyBasedTableModel *  lensTableModel;
   QRegExpValidator *         validatorName;
};
