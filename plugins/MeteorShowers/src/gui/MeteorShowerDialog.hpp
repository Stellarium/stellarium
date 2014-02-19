/*
 * Stellarium Meteor Shower Plug-in GUI
 * 
 * Copyright (C) 2013 Marcos Cardinot
 * Copyright (C) 2011 Alexander Wolf
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
 
#ifndef _METEORSHOWERDIALOG_HPP_
#define _METEORSHOWERDIALOG_HPP_

#include <QColor>
#include <QLabel>
#include <QObject>
#include <QStandardItemModel>

#include "MeteorShowers.hpp"
#include "StelDialog.hpp"

class MeteorShowers;
class QTimer;
class Ui_meteorShowerDialog;

class MeteorShowerDialog : public StelDialog
{
	Q_OBJECT

public:
	MeteorShowerDialog();
	~MeteorShowerDialog();

protected:
	//! Initialize the dialog widgets and connect the signals/slots
	void createDialogContent();

public slots:
	void retranslate();
	void refreshUpdateValues(void);
	void refreshColorMarkers(void);
	void refreshRangeDates(void); //! Refresh dates range when year in main app change

private slots:
	void setUpdateValues(int hours);
	void setUpdatesEnabled(bool checkState);
	void updateStateReceiver(MeteorShowers::UpdateState state);
        void updateCompleteReceiver();
	void restoreDefaults(void);
	void saveSettings(void);
	void updateJSON(void);
	void setColorARR(void); //! Set color of active radiant based on real data.
	void setColorARG(void); //! Set color of active radiant based on generic data.
	void setColorIR(void); //! Set color of inactive radiant.
	void checkDates(void);
	void searchEvents(void);
	void selectEvent(const QModelIndex &modelIndex);

private:
        Ui_meteorShowerDialog* ui;
	MeteorShowers* plugin;
	void setAboutHtml(void);
	void updateGuiFromSettings(void);
	QTimer* updateTimer;
	void setTextureColor(QLabel* texture, QColor color);

	//! Defines the number and the order of the columns in the table that lists active meteor showers
	//! @enum ModelColumns
	enum ModelColumns {
		ColumnName,		//!< name column
		ColumnZHR,		//!< zhr column
		ColumnDataType,		//!< data type column
		ColumnPeak,		//!< peak date column
		ColumnCount		//!< total number of columns
	};
	QStandardItemModel * listModel;
	void initListEvents(void);
	void setHeaderNames(void);
};

#endif // _METEORSHOWERDIALOG_HPP_
