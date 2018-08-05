/*
 * Copyright (C) 2010 Timothy Reaves
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

#ifndef OBSERVATIONSDIALOG_HPP
#define OBSERVATIONSDIALOG_HPP

#include <QObject>
#include <QMap>
#include "StelDialogLogBook.hpp"
#include "StelStyle.hpp"

class Ui_ObservationsDialog;
class FieldConcatModel;
class QDateTime;
class QModelIndex;
class QSqlRecord;
class QSqlRelationalTableModel;
class QSqlTableModel;

class ObservationsDialog : public StelDialogLogBook {
	Q_OBJECT
	
public:
	ObservationsDialog(QMap<QString, QSqlTableModel *> theTableModels, int sessionID);
	virtual ~ObservationsDialog();
	void languageChanged();
	//! Notify that the application style changed
	void styleChanged();
	void updateStyle();
	
protected slots:
	void accessoriesTextChanged();
	void barlowChanged(const QString &newValue);
	void beginDateTimeChanged(const QDateTime &datetime);
	void deleteSelectedObservation();
	void endDateTimeChanged(const QDateTime &datetime);
	void filterChanged(const QString &newValue);
	void imagerChanged(const QString &newValue);
	void insertNewObservation();
	void limitingMagnitudeChanged(const QString &newValue);
	void notesTextChanged();
	void observerChanged(const QString &newValue);
	void observationSelected(const QModelIndex &index);
	void ocularChanged(const QString &newValue);
	void opticChanged(const QString &newValue);
	void seeingChanged(const QString &newValue);
	void targetChanged(const QString &newValue);
	
protected:
	//! Initialize the dialog widgets and connect the signals/slots
	virtual void createDialogContent();
	QSqlRecord currentObservationRecord();
	void populateFormWithObservationIndex(const QModelIndex &index);
	void setupConnections();
	void setupModels();
	void teardownConnections();
	
private:
	QMap<QString, FieldConcatModel *>	fieldModels;
	int									lastObservationRowNumberSelected;
	QSqlRelationalTableModel*			observationsModel;
	int									sessionKey;
	QMap<QString, QSqlTableModel *>		tableModels;
	Ui_ObservationsDialog*				ui;
};

#endif // OBSERVATIONSDIALOG_HPP
