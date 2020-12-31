/*
 * Stellarium Exoplanets Plug-in GUI
 * 
 * Copyright (C) 2012 Alexander Wolf
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
 
#ifndef EXOPLANETSDIALOG_HPP
#define EXOPLANETSDIALOG_HPP

#include <QObject>
#include <QTreeWidget>
#include <QTreeWidgetItem>

#include "StelDialog.hpp"
#include "Exoplanets.hpp"
#include "StelObjectMgr.hpp"
#include "StelMovementMgr.hpp"

class Ui_exoplanetsDialog;
class QTimer;
class QToolButton;
class Exoplanets;

//! @ingroup exoplanets
//! Main window of the %Exoplanets plugin.
class ExoplanetsDialog : public StelDialog
{
	Q_OBJECT

public:
	//! @enum ExoplanetsColumns
	enum ExoplanetsColumns {
		EPSExoplanetName,
		EPSExoplanetMass,
		EPSExoplanetRadius,
		EPSExoplanetPeriod,
		EPSExoplanetSemiAxes,
		EPSExoplanetEccentricity,
		EPSExoplanetInclination,
		EPSExoplanetAngleDistance,		
		EPSStarMagnitude,
		EPSStarRadius,
		EPSExoplanetDetectionMethod,
		EPSCount            //! total number of columns
	};

	ExoplanetsDialog();
	~ExoplanetsDialog();

protected:
	//! Initialize the dialog widgets and connect the signals/slots
	void createDialogContent();

public slots:
	void retranslate();
	void refreshUpdateValues(void);

private slots:
	void setUpdateValues(int hours);
	void setUpdatesEnabled(int checkState);
	void setDistributionEnabled(int checkState);
	void setTimelineEnabled(int checkState);
	void setHabitableEnabled(int checkState);
	void setDisplayAtStartupEnabled(int checkState);
	void setDisplayShowExoplanetsButton(int checkState);
	void setDisplayShowExoplanetsDesignations(int checkState);
	void updateStateReceiver(Exoplanets::UpdateState state);
        void updateCompleteReceiver();
	void restoreDefaults(void);
	void saveSettings(void);
	void updateJSON(void);
	void drawDiagram(void);
	void populateDiagramsList();

	void populateTemperatureScales();
	void setTemperatureScale(int tScaleID);
	void selectCurrentExoplanet(const QModelIndex &modelIndex);

private:
        Ui_exoplanetsDialog* ui;
	Exoplanets* ep;
	class StelObjectMgr* objectMgr;
	class StelMovementMgr* mvMgr;
	void setAboutHtml(void);
	void setInfoHtml(void);
	void setWebsitesHtml(void);
	void updateGuiFromSettings(void);
	QTimer* updateTimer;

	//! Update names for table columns
	void setColumnNames();
	void fillExoplanetsTable();

	QStringList exoplanetsHeader;
	typedef QPair<QString, int> axisPair;
};

// Reimplements the QTreeWidgetItem class to fix the sorting bug
class EPSTreeWidgetItem : public QTreeWidgetItem
{
public:
	EPSTreeWidgetItem(QTreeWidget* parent)
		: QTreeWidgetItem(parent)
	{
	}

private:
	bool operator < (const QTreeWidgetItem &other) const
	{
		int column = treeWidget()->sortColumn();

		if (column == ExoplanetsDialog::EPSExoplanetName || column == ExoplanetsDialog::EPSExoplanetDetectionMethod)
			return text(column).toLower() < other.text(column).toLower();
		else
			return text(column).toFloat() < other.text(column).toFloat();
	}
};

#endif // EXOPLANETSDIALOG_HPP
