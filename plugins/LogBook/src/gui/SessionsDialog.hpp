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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef _SESSIONSDIALOG_HPP_
#define _SESSIONSDIALOG_HPP_

#include <QObject>
#include <QMap>
#include "StelDialogLogBook.hpp"
#include "StelStyle.hpp"

class Ui_SessionsDialog;
class FieldConcatModel;
class QDateTime;
class QModelIndex;
class QSqlRecord;
class QSqlRelationalTableModel;
class QSqlTableModel;

class SessionsDialog : public StelDialogLogBook {
	Q_OBJECT
	
public:
	SessionsDialog(QMap<QString, QSqlTableModel *> theTableModels);
	virtual ~SessionsDialog();
	void languageChanged();
	//! Notify that the application style changed
	void styleChanged();
	void updateStyle();
	
protected slots:
	// session slots
	void beginDateTimeChanged(const QDateTime &datetime);
	void commentsTextChanged();
	void deleteSelectedSession();
	void endDateTimeChanged(const QDateTime &datetime);
	void insertNewSession();
	void observationWindowClosed(StelDialogLogBook* theDialog);
	void observerChanged(const QString &newValue);
	void openObservations();
	void sessionSelected(const QModelIndex &index);
	void siteChanged(const QString &newValue);
	void weatherTextChanged();
	
protected:
	//! Initialize the dialog widgets and connect the signals/slots
	virtual void createDialogContent();
	QSqlRecord currentSessionRecord();
	void populateFormWithSessionIndex(const QModelIndex &index);
	void setupConnections();
	void setupModels();
	void teardownConnections();
	
private:
	QMap<QString, FieldConcatModel *>	fieldModels;
	int									lastSessionRowNumberSelected;
	QSqlRelationalTableModel*			sessionsModel;
	QMap<QString, QSqlTableModel *>		tableModels;
	Ui_SessionsDialog*					ui;
};

#endif // _SESSIONSDIALOG_HPP_
