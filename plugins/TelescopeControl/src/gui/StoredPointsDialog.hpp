/*
 * Stellarium Telescope Control Plug-in
 *
 * Copyright (C) 2015 Pavel Klimenko aka rich <dzy4@mail.ru> (this file)
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

#ifndef STOREDPOINTS_HPP
#define STOREDPOINTS_HPP

#include <QDialog>
#include <QStandardItemModel>

#include "StelApp.hpp"
#include "StelStyle.hpp"
#include "StelTranslator.hpp"
#include "StelDialog.hpp"
#include "StelCore.hpp"
#include "StelObjectMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelUtils.hpp"

#include "AngleSpinBox.hpp"

class Ui_StoredPoints;

struct storedPoint
{
	int number;
	QString name;
	double radiansRA;
	double radiansDec;
};
Q_DECLARE_METATYPE(storedPoint)

class StoredPointsDialog : public StelDialog
{
	Q_OBJECT

public:
	StoredPointsDialog();
	~StoredPointsDialog();

	void populatePointsList(QVariantList list);

public slots:
	void retranslate();

private slots:
	void buttonAddPressed();
	void buttonRemovePressed();
	void buttonClearPressed();

	void getCurrentObjectInfo();
	void getCenterInfo();

signals:
	void addStoredPoint(int number, QString name, double radiansRA, double radiansDec);
	void removeStoredPoint(int number);
	void clearStoredPoints();

protected:
	//! Initialize the dialog widgets and connect the signals/slots
	virtual void createDialogContent();
	Ui_StoredPoints *ui;
private:
	//! @enum ModelColumns This enum defines the number and the order of the columns in the table that lists points

	enum ModelColumns
	{
		ColumnSlot = 0,	//!< slot number column
		ColumnName,	//!< point name column
		ColumnRA,	//!< point ra_j2000 column
		ColumnDec,	//!< point dec_j2000 column
		ColumnCount,	//!< total number of columns
	};
	QStandardItemModel * storedPointsListModel;

	void setHeaderNames();
	void addModelRow(int number, QString name, QString RA, QString Dec);
};

#endif // STOREDPOINTS_HPP
