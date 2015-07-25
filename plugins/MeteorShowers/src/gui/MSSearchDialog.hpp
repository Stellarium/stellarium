/*
 * Stellarium: Meteor Showers Plug-in
 * Copyright (C) 2013-2015 Marcos Cardinot
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
 
#ifndef _MSSEARCHDIALOG_HPP_
#define _MSSEARCHDIALOG_HPP_

#include <QColor>
#include <QLabel>
#include <QObject>
#include <QTreeWidget>

#include "MeteorShowers.hpp"
#include "StelDialog.hpp"

class Ui_MSSearchDialog;
class MeteorShowers;

//! @class MSSearchDialog
//! @author Marcos Cardinot <mcardinot@gmail.com>
//! @ingroup meteorShowers
class MSSearchDialog : public StelDialog
{
	Q_OBJECT

public:
	//! Defines the number and the order of the columns in the table that lists active meteor showers
	//! @enum ModelColumns
	enum ModelColumns {
		ColumnName,		//! name column
		ColumnZHR,		//! zhr column
		ColumnDataType,		//! data type column
		ColumnPeak,		//! peak date column
		ColumnCount		//! total number of columns
	};

	//! Constructor
	MSSearchDialog(MeteorShowersMgr *mgr);

	//! Destructor
	~MSSearchDialog();

protected:
	//! Initialize the dialog and connect the signals/slots
	void createDialogContent();

public slots:
	void retranslate();
	void refreshRangeDates(StelCore *core);   //! Refresh dates range when year in main app change

private slots:
	void checkDates(void);   //! Checks if the inputed dates are valid for use.
	void searchEvents(void); //! Search events and fill the list.
	void selectEvent(const QModelIndex &modelIndex); //! If an event is selected by user, the current date change and the object is selected.
	void repaintTreeWidget();

private:
	MeteorShowersMgr* m_mgr;
	Ui_MSSearchDialog* m_ui;
	QTreeWidget* m_treeWidget;     //! list of events

	void initListEvents(); //! Init header and list of events
	void setHeaderNames(); //! Update header names
};

// Reimplements the QTreeWidgetItem class to fix the sorting bug
class TreeWidgetItem : public QTreeWidgetItem
{
public:
	TreeWidgetItem(QTreeWidget* parent):QTreeWidgetItem(parent){}

private:
	bool operator<(const QTreeWidgetItem &other)const {
		int column = treeWidget()->sortColumn();

		if (column == MSSearchDialog::ColumnPeak)
			return QDateTime::fromString(text(column),"dd/MMM/yyyy").operator <(QDateTime::fromString(other.text(column),"dd/MMM/yyyy"));
		else if (column == MSSearchDialog::ColumnZHR)
			return text(column).toInt() < other.text(column).toInt();
		else //ColumnName or ColumnDataType
			return text(column).toLower() < other.text(column).toLower();
	}
};

#endif // _MSSEARCHDIALOG_HPP_
