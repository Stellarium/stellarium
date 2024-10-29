/*
 * Stellarium Satellites Plug-in: satellites communications editor
 * Copyright (C) 2022 Alexander Wolf
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef SATELLITESCOMMDIALOG_HPP
#define SATELLITESCOMMDIALOG_HPP

#include "StelDialog.hpp"
#include "Satellites.hpp"

#include <QTreeWidget>
#include <QTreeWidgetItem>

class Ui_satellitesCommDialog;

//! @ingroup satellites
class SatellitesCommDialog : public StelDialog
{
	Q_OBJECT
	
public:
	enum CommsColumns {
		CommsDescription, //! name of communication link
		CommsFrequency,   //! frequency, MHz
		CommsModulation,  //! modulation info
		CommsCount        //! total number of columns
	};

	SatellitesCommDialog();
	~SatellitesCommDialog() override;
	
public slots:
	void retranslate() override;
	void setVisible(bool visible = true) override;

private slots:
	void updateSatID(const QString &satID);
	void selectCurrentCommLink();

	void addCommData();
	void removeCommData();

protected:
	//! Initialize the dialog widgets and connect the signals/slots.
	void createDialogContent() override;
	Ui_satellitesCommDialog* ui;

private:
	class Satellites* SatellitesMgr;

	//! Update header names for communications table
	void setCommunicationsHeaderNames();
	void adjustCommunicationsColumns();
	//! Init header and list of communications
	void initListCommunications();
	void fillCommunicationsTable(const QString &satID, const QString &description, double frequency, const QString &modulation);

	void getSatCommData();

	void populateTexts();

	QList<CommLink> communications;
	QStringList communicationsHeader;
	QString satelliteID;
};

// Reimplements the QTreeWidgetItem class to fix the sorting bug
class CommsTreeWidgetItem : public QTreeWidgetItem
{
public:
	CommsTreeWidgetItem(QTreeWidget* parent)
		: QTreeWidgetItem(parent)
	{
	}

private:
	bool operator < (const QTreeWidgetItem &other) const override
	{
		int column = treeWidget()->sortColumn();
		if (column == SatellitesCommDialog::CommsDescription)
			return text(column).toFloat() < other.text(column).toFloat();
		else
			return text(column).toLower() < other.text(column).toLower();
	}
};

#endif // SATELLITESCOMMDIALOG_HPP
