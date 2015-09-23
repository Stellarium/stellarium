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

#include <QTreeWidget>

#include "MeteorShowers.hpp"
#include "StelDialog.hpp"

class Ui_MSSearchDialog;

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

private slots:
	//! Checks if the inputed dates are valid for use.
	void checkDates();

	//! Search events and fill the list.
	void searchEvents();

	//! If an event is selected by user, the current date change and the object is selected.
	void selectEvent(const QModelIndex &modelIndex);

	//! Refresh dates
	//! from Jan 1 to Dec 31 for the current sky year
	void refreshRangeDates();

private:
	MeteorShowersMgr* m_mgr;
	Ui_MSSearchDialog* m_ui;

	//! Init header and list of events
	void initListEvents();

	//! Update header names
	void setHeaderNames();
};

// Reimplements the QTreeWidgetItem class to fix the sorting bug
class MSTreeWidgetItem : public QTreeWidgetItem
{
public:
	MSTreeWidgetItem(QTreeWidget* parent)
		: QTreeWidgetItem(parent)
	{
	}

private:
	bool operator < (const QTreeWidgetItem &other) const
	{
		int column = treeWidget()->sortColumn();

		if (column == MSSearchDialog::ColumnPeak)
		{
			QDateTime a = QDateTime::fromString(text(column),"dd/MMM/yyyy");
			QDateTime b = QDateTime::fromString(other.text(column),"dd/MMM/yyyy");
			return a.operator < (b);
		}
		else if (column == MSSearchDialog::ColumnZHR)
		{
			// the zhr can be variable ("min-max")
			QStringList as = text(column).split("-");
			QStringList bs = other.text(column).split("-");
			// in case of "variable", choose max value (1)
			int a = as.size() == 1? as.at(0).toInt() : as.at(1).toInt();
			int b = bs.size() == 1? bs.at(0).toInt() : bs.at(1).toInt();
			return a < b;
		}
		else //ColumnName or ColumnDataType
		{
			return text(column).toLower() < other.text(column).toLower();
		}
	}
};

#endif // _MSSEARCHDIALOG_HPP_
