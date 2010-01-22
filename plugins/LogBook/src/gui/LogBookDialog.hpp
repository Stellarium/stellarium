/*
 * Copyright (C) 2009 Timothy Reaves
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef _LOGBOOKDIALOG_HPP_
#define _LOGBOOKDIALOG_HPP_

#include <QObject>
#include <QMap>
#include "StelDialogLogBook.hpp"
#include "StelStyle.hpp"

class Ui_LogBookDialogForm;
class FieldConcatModel;
class QDateTime;
class QModelIndex;
class QSqlRecord;
class QSqlRelationalTableModel;
class QSqlTableModel;

class LogBookDialog : public StelDialogLogBook {
	Q_OBJECT
	
public:
	LogBookDialog(QMap<QString, QSqlTableModel *> theTableModels);
	virtual ~LogBookDialog();
	void languageChanged();
	virtual void setStelStyle(const StelStyle& style);
	
	//! Notify that the application style changed
	void styleChanged();
	
protected slots:
	void closeWindow();

	// session slots
	void commentsTextChanged();
	void deleteSelectedSession();
	void insertNewSession();
	void sessionBeginDateTimeChanged(const QDateTime &datetime);
	void sessionEndDateTimeChanged(const QDateTime &datetime);
	void sessionObserverChanged(const QString &newValue);
	void sessionSelected(const QModelIndex &index);
	void siteChanged(const QString &newValue);
	void weatherTextChanged();

	// observation slots
	void accessoriesTextChanged();
	void barlowChanged(const QString &newValue);
	void deleteSelectedObservation();
	void filterChanged(const QString &newValue);
	void imagerChanged(const QString &newValue);
	void insertNewObservation();
	void limitingMagnitudeChanged(const QString &newValue);
	void notesTextChanged();
	void observationBeginDateTimeChanged(const QDateTime &datetime);
	void observationEndDateTimeChanged(const QDateTime &datetime);
	void observationObserverChanged(const QString &newValue);
	void observationSelected(const QModelIndex &index);
	void ocularChanged(const QString &newValue);
	void opticChanged(const QString &newValue);
	void seeingChanged(const QString &newValue);
	void targetChanged(const QString &newValue);

protected:
	//! Initialize the dialog widgets and connect the signals/slots
	virtual void createDialogContent();
	QSqlRecord currentObservationRecord();
	QSqlRecord currentSessionRecord();
	void populateFormWithObservationIndex(const QModelIndex &index);
	void populateFormWithSessionIndex(const QModelIndex &index);
	void setupConnections();
	void setupModels();
	void teardownConnections();
	
private:
	int lastObservationRowNumberSelected;
	int lastSessionRowNumberSelected;
	QMap<QString, QSqlTableModel *> tableModels;
	QMap<QString, FieldConcatModel *> fieldModels;
	QSqlRelationalTableModel *observationsModel;
	FieldConcatModel *observationsListModel;
	Ui_LogBookDialogForm* ui;
};

#endif // _LOGBOOKDIALOG_HPP_
