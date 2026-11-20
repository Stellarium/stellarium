/*
 * Stellarium
 * Copyright (C) 2026 Sylvain Simard
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
 * 
 * Dialog of the Earth Orientation Parameters (EOP): files, model, current values and table
 * (SS) 2026-10-07 Modelled on TTminusTDBKernelDialog
*/

#ifndef EOPDIALOG_HPP
#define EOPDIALOG_HPP

#include "StelDialog.hpp"

#include <QObject>
#include <QStringList>

class Ui_eopDialogForm;
class StelCore;
class QCheckBox;
class QTableWidgetItem;
class QTimer;

//! @class EOPDialog
//! Dialog of the Earth Orientation Parameters (EOP) published by the IERS. Opened from the wrench button next to the EOP
//! checkbox of the Tools tab of the Configuration dialog. It shows and selects the files of the "eop" folder, downloads them
//! again from the IERS (Update button), selects the nutation model that decides the merge order of the files
//! (IAU 1980 as JPL Horizons, or IAU 2000A), shows the values of the selected EOP fields at the date of the simulation
//! (kept up to date while time is running), the last valid value of every EOP, and a table of the daily records between
//! two dates that can be saved in a .csv file.
class EOPDialog : public StelDialog
{
	Q_OBJECT

public:
	EOPDialog();
	~EOPDialog() override;

public slots:
	void retranslate() override;

protected:
	//! Initialize the dialog widgets and connect the signals/slots.
	void createDialogContent() override;
	Ui_eopDialogForm* ui;

private slots:
	//! Fill again everything that depends on the merged files
	void refreshAll();
	void populateFilesTable();
	void fileItemChanged(QTableWidgetItem* item);
	void modelChanged(int index);
	void syncModel();
	void openFolderClicked();
	void rescanClicked();
	void defaultFilesClicked();
	void updateClicked();
	void updateProgress(qint64 received, qint64 total);
	void updateFinished(bool ok, const QStringList& written, const QString& message);
	void autoUpdateToggled(bool on);
	void updateUpdateStatus();
	void presetClicked(const QString& mode);
	void fieldCheckBoxClicked();
	void updateLiveValues();
	//! Called by a timer: refresh the live values when the Values tab is shown and the simulation time has moved.
	void updateLiveIfVisible();
	void updateSummary();
	void updateDateRanges();
	void showTable();
	void currentDateClicked();
	void saveCsv();
	void tabChanged(int index);

private:
	StelCore* core;
	bool populating;
	QTimer* liveTimer;
	double lastLiveJD;
	QString lastUpdateMessage;
	QString fieldsMode;				//!< "all", "default", "short", "none" or "custom"
	QStringList customFields;			//!< fields of the customized selection
	QList<QCheckBox*> fieldBoxes;			//!< one check box per EOP field, in the order of the field list

	void setDescriptions();
	void populateModelCombo();
	void setTableHeaders();
	QStringList presetFields(const QString& mode) const;
	QStringList selectedFields() const;
	void setFieldCheckBoxes(const QStringList& ids);
	void setPresetRadioButton(const QString& mode);
};

#endif // EOPDIALOG_HPP
