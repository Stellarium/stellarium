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

#ifndef _TARGETSDIALOG_HPP_
#define _TARGETSDIALOG_HPP_

#include <QObject>
#include <QMap>

#include "StelDialogLogBook.hpp"
#include "StelStyle.hpp"

class Ui_TargetsDialogForm;
class QModelIndex;
class QSqlRecord;
class QSqlTableModel;

class TargetsDialog : public StelDialogLogBook {
	Q_OBJECT
	
public:
	TargetsDialog(QMap<QString, QSqlTableModel *> theTableModels);
	virtual ~TargetsDialog();
	void languageChanged();
	//! Notify that the application style changed
	void styleChanged();
	void updateStyle();
	
public slots:
	void aliasChanged();
	void closeWindow();
	void catalogNumberChanged();
	void createTargetFromSelection();
	void declinationChanged();
	void deleteSelectedTarget();
	void distanceChanged();
	void insertNewTarget();
	void magnitudeChanged();
	void nameChanged();
	void notesChanged();
	void rightAscentionChanged();
	void sizeChanged();
	void targetSelected(const QModelIndex &index);
	void targetTypeChanged(const QString &newValue);
	void updateRTValues();
	
protected:
	//! Initialize the dialog widgets and connect the signals/slots
	virtual void createDialogContent();
	QSqlRecord currentRecord();
	void populateFormWithIndex(const QModelIndex &index);
	void setupConnections();
	void teardownConnections();

private:
	int lastRowNumberSelected;
	Ui_TargetsDialogForm* ui;
	QSqlTableModel *tableModel;
	QSqlTableModel *typeTableModel;

};

#endif // _TARGETSDIALOG_HPP_
