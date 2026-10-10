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

#include "EOPDialog.hpp"
#include "ui_eopDialog.h"

#include "Dialog.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelFileMgr.hpp"
#include "StelMainView.hpp"
#include "StelTranslator.hpp"
#include "EOPManager.hpp"

#include <QCheckBox>
#include <QDate>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QGridLayout>
#include <QHeaderView>
#include <QProgressBar>
#include <QSaveFile>
#include <QSettings>
#include <QTableWidget>
#include <QTimer>
#include <QUrl>
#include <algorithm>
#include <cmath>

namespace
{
// The EOP fields, as the columns of the IERS files. Dates and types (final or prediction) are text, the other fields are numbers.
struct FieldDef
{
	const char* id;
	const char* label;
	const char* unit;
	int decimals;
};

const FieldDef FIELDS[] = {
	{"mjd",      "MJD",                     "",       0},
	{"date",     "Date (YYYY-MM-DD)",       "",       0},
	{"poleType", "Type of pole values",     "",       0},
	{"xp",       "x_p",                     "arcsec", 6},
	{"sxp",      "sigma x_p",               "arcsec", 6},
	{"yp",       "y_p",                     "arcsec", 6},
	{"syp",      "sigma y_p",               "arcsec", 6},
	{"utType",   "Type of UT values",       "",       0},
	{"ut1",      "UT1-UTC",                 "s",      7},
	{"sut1",     "sigma UT1-UTC",           "s",      7},
	{"lod",      "LOD",                     "s",      7},
	{"slod",     "sigma LOD",               "s",      7},
	{"nutType",  "Type of nutation values", "",       0},
	{"dpsi",     "dPsi (IAU 1980)",         "arcsec", 6},
	{"sdpsi",    "sigma dPsi",              "arcsec", 6},
	{"deps",     "dEpsilon (IAU 1980)",     "arcsec", 6},
	{"sdeps",    "sigma dEpsilon",          "arcsec", 6},
	{"dx",       "dX (IAU 2000A)",          "arcsec", 6},
	{"sdx",      "sigma dX",                "arcsec", 6},
	{"dy",       "dY (IAU 2000A)",          "arcsec", 6},
	{"sdy",      "sigma dY",                "arcsec", 6}
};
const int N_FIELDS = int(sizeof(FIELDS) / sizeof(FIELDS[0]));
const int TABLE_MAX_ROWS = 5000;      // rows shown in the table of the dialog
const int CSV_MAX_RECORDS = 200000;   // records of a .csv file

QString fieldId(int i) { return QString::fromLatin1(FIELDS[i].id); }
bool isValueField(int i) { return FIELDS[i].unit[0] != '\0'; }

QString fieldLabel(int i)
{
	QString s = QString::fromLatin1(FIELDS[i].label);
	if (FIELDS[i].unit[0] != '\0')
		s += QString(" [%1]").arg(QString::fromLatin1(FIELDS[i].unit));
	return s;
}

QString dateText(double mjd)
{
	return QDate::fromJulianDay(qint64(std::floor(mjd + 1e-9)) + 2400001).toString("yyyy-MM-dd");
}

QDate mjdToDate(double mjd)
{
	return QDate::fromJulianDay(qint64(std::floor(mjd + 1e-9)) + 2400001);
}

double dateToMjd(const QDate& d)
{
	return double(d.toJulianDay() - 2400001);
}

// Text of a field for a record ("-" if the record has no value for it)
QString fieldText(const QString& id, const EOPReader::Record& r, int mjdDecimals)
{
	auto num = [](bool has, double v, int decimals) { return has ? QString::number(v, 'f', decimals) : QStringLiteral("-"); };
	if (id == "mjd")      return QString::number(r.mjd, 'f', mjdDecimals);
	if (id == "date")     return dateText(r.mjd);
	if (id == "poleType") return EOPReader::statusName(r.poleStatus);
	if (id == "utType")   return EOPReader::statusName(r.utStatus);
	if (id == "nutType")  return EOPReader::statusName(r.nutationStatus);
	if (id == "xp")       return num(r.hasPole, r.xp, 6);
	if (id == "sxp")      return num(r.hasPole, r.xpErr, 6);
	if (id == "yp")       return num(r.hasPole, r.yp, 6);
	if (id == "syp")      return num(r.hasPole, r.ypErr, 6);
	if (id == "ut1")      return num(r.hasUT, r.ut1utc, 7);
	if (id == "sut1")     return num(r.hasUT, r.ut1utcErr, 7);
	if (id == "lod")      return num(r.hasLOD, r.lod, 7);
	if (id == "slod")     return num(r.hasLOD, r.lodErr, 7);
	if (id == "dpsi")     return num(r.hasNutation, r.dpsi, 6);
	if (id == "sdpsi")    return num(r.hasNutation, r.dpsiErr, 6);
	if (id == "deps")     return num(r.hasNutation, r.deps, 6);
	if (id == "sdeps")    return num(r.hasNutation, r.depsErr, 6);
	if (id == "dx")       return num(r.hasDXDY, r.dx, 6);
	if (id == "sdx")      return num(r.hasDXDY, r.dxErr, 6);
	if (id == "dy")       return num(r.hasDXDY, r.dy, 6);
	if (id == "sdy")      return num(r.hasDXDY, r.dyErr, 6);
	return QString();
}

// How the value of a field at the date of the simulation was obtained
QString statusText(const QString& id, const EOPReader::Interpolated& e)
{
	const EOPReader::Record& r = e.rec;
	bool has = false, held = false;
	EOPReader::Status st = EOPReader::Status::Unknown;
	if (id == "xp" || id == "sxp" || id == "yp" || id == "syp")
	{
		has = r.hasPole; held = e.poleHeld; st = r.poleStatus;
	}
	else if (id == "ut1" || id == "sut1")
	{
		has = r.hasUT; held = e.utHeld; st = r.utStatus;
	}
	else if (id == "lod" || id == "slod")
	{
		has = r.hasLOD; held = e.lodHeld; st = r.lodStatus;
	}
	else if (id == "dpsi" || id == "sdpsi" || id == "deps" || id == "sdeps")
	{
		has = r.hasNutation; held = e.nutationHeld; st = r.nutationStatus;
	}
	else
	{
		has = r.hasDXDY; held = e.dxdyHeld; st = r.dxdyStatus;
	}
	if (!has)
		return q_("no value");
	return QString("%1, %2").arg(held ? q_("held (last value)") : q_("interpolated"), EOPReader::statusName(st));
}

QString shortModelName(EOPManager::Model m)
{
	switch (m)
	{
		case EOPManager::Model::IAU1980:  return QStringLiteral("IAU 1980");
		case EOPManager::Model::IAU2000A: return QStringLiteral("IAU 2000A");
		default:                          return QStringLiteral("?");
	}
}

QTableWidgetItem* cell(const QString& text, Qt::Alignment alignment = Qt::AlignLeft | Qt::AlignVCenter)
{
	QTableWidgetItem* item = new QTableWidgetItem(text);
	item->setTextAlignment(alignment);
	return item;
}
}

EOPDialog::EOPDialog()
	: StelDialog("EOP")
	, ui(Q_NULLPTR)
	, populating(false)
	, liveTimer(Q_NULLPTR)
	, lastLiveJD(0.)
	, fieldsMode("default")
{
	ui = new Ui_eopDialogForm;
	core = StelApp::getInstance().getCore();
}

EOPDialog::~EOPDialog()
{
	delete ui;
	ui = Q_NULLPTR;
}

void EOPDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		setDescriptions();
		populateModelCombo();
		setTableHeaders();
		refreshAll();
	}
}

void EOPDialog::setDescriptions()
{
	ui->titleBar->setTitle(q_("Earth Orientation Parameters (EOP)"));
	ui->labelFilesHelp->setText(q_("Tick the files to merge. They are merged in the order of the Merge column: where several files have a value for "
				       "a day, the last one wins. The files of the selected model come last. Series: Long term = EOP C04 (from 1962), "
				       "Standard = finals (from 1973, with predictions)."));
	ui->labelFieldsHelp->setText(q_("The fields chosen here are used by the Values tab, the Table tab and the .csv file. In the Values tab, "
					"dates and types are shown in the heading and in the Status column."));
}

void EOPDialog::populateModelCombo()
{
	populating = true;
	ui->modelComboBox->clear();
	ui->modelComboBox->addItem(q_("IAU 1980 (as JPL Horizons)"), "IAU1980");
	ui->modelComboBox->addItem(q_("IAU 2000A"), "IAU2000A");
	populating = false;
	syncModel();
}

void EOPDialog::setTableHeaders()
{
	ui->filesTable->setColumnCount(8);
	ui->filesTable->setHorizontalHeaderLabels(QStringList() << q_("Use") << q_("Merge") << q_("File") << q_("Series") << q_("Model")
						  << q_("Downloaded") << q_("Coverage") << q_("Records"));
	ui->filesTable->horizontalHeader()->setStretchLastSection(true);
	ui->liveTable->setColumnCount(4);
	ui->liveTable->setHorizontalHeaderLabels(QStringList() << q_("Field") << q_("Value") << q_("Unit") << q_("Status"));
	ui->liveTable->horizontalHeader()->setStretchLastSection(true);
}

void EOPDialog::createDialogContent()
{
	ui->setupUi(dialog);
	setDescriptions();
	setTableHeaders();
	populateModelCombo();

	// one check box per EOP field, in three columns
	const int rows = (N_FIELDS + 2) / 3;
	for (int i = 0; i < N_FIELDS; ++i)
	{
		QCheckBox* box = new QCheckBox(fieldLabel(i), ui->fieldsGroup);
		fieldBoxes << box;
		ui->fieldsGridLayout->addWidget(box, i % rows, i / rows);
		connect(box, &QCheckBox::clicked, this, [this]() { fieldCheckBoxClicked(); });
	}

	// settings of the previous session
	QSettings* conf = StelApp::getInstance().getSettings();
	fieldsMode = conf->value("astro/eop_fields_mode", "default").toString();
	if (fieldsMode != "all" && fieldsMode != "default" && fieldsMode != "short" && fieldsMode != "none" && fieldsMode != "custom")
		fieldsMode = "default";
	customFields = conf->value("astro/eop_fields", "").toString().split(QLatin1Char(','), Qt::SkipEmptyParts);
	if (customFields.isEmpty())
		customFields = presetFields("default");
	setPresetRadioButton(fieldsMode);
	setFieldCheckBoxes(presetFields(fieldsMode));
	ui->checkBoxAutoUpdate->setChecked(conf->value("astro/eop_auto_update", true).toBool());

	// Signals and slots
	connect(&StelApp::getInstance(), &StelApp::languageChanged, this, &EOPDialog::retranslate);
	connect(ui->titleBar, &TitleBar::closeClicked, this, &StelDialog::close);
	connect(ui->titleBar, &TitleBar::movedTo,      this, &EOPDialog::handleMovedTo);

	connectBoolProperty(ui->applyCheckBox, "StelCore.flagUseEOP");
	connect(core, &StelCore::flagUseEOPChanged, this, [this]() { updateLiveValues(); });
	connect(core, &StelCore::eopFilesChanged,   this, [this]() { refreshAll(); });
	connect(core, &StelCore::eopModelChanged,   this, [this]()
	{
		if (fieldsMode == "short") // the short selection depends on the model
			setFieldCheckBoxes(presetFields("short"));
		syncModel();
	});
	connect(core, &StelCore::eopUpdateProgress, this, &EOPDialog::updateProgress);
	connect(core, &StelCore::eopUpdateFinished, this, &EOPDialog::updateFinished);
	connect(core, &StelCore::dateChanged,       this, &EOPDialog::updateLiveIfVisible);
	connect(core, &StelCore::timeSyncOccurred,  this, &EOPDialog::updateLiveIfVisible);

	connect(ui->modelComboBox, qOverload<int>(&QComboBox::currentIndexChanged), this, &EOPDialog::modelChanged);
	connect(ui->pushButtonOpenFolder,   &QPushButton::clicked, this, &EOPDialog::openFolderClicked);
	connect(ui->pushButtonRescan,       &QPushButton::clicked, this, &EOPDialog::rescanClicked);
	connect(ui->pushButtonDefaultFiles, &QPushButton::clicked, this, &EOPDialog::defaultFilesClicked);
	connect(ui->pushButtonUpdate,       &QPushButton::clicked, this, &EOPDialog::updateClicked);
	connect(ui->checkBoxAutoUpdate,     &QCheckBox::toggled,   this, &EOPDialog::autoUpdateToggled);
	connect(ui->filesTable, &QTableWidget::itemChanged, this, &EOPDialog::fileItemChanged);

	connect(ui->radioAll,     &QRadioButton::clicked, this, [this]() { presetClicked("all"); });
	connect(ui->radioDefault, &QRadioButton::clicked, this, [this]() { presetClicked("default"); });
	connect(ui->radioShort,   &QRadioButton::clicked, this, [this]() { presetClicked("short"); });
	connect(ui->radioNone,    &QRadioButton::clicked, this, [this]() { presetClicked("none"); });
	connect(ui->radioCustom,  &QRadioButton::clicked, this, [this]() { presetClicked("custom"); });

	connect(ui->pushButtonShowTable,   &QPushButton::clicked, this, &EOPDialog::showTable);
	connect(ui->pushButtonCurrentDate, &QPushButton::clicked, this, &EOPDialog::currentDateClicked);
	connect(ui->pushButtonSaveCsv,     &QPushButton::clicked, this, &EOPDialog::saveCsv);
	connect(ui->tabWidget, &QTabWidget::currentChanged, this, &EOPDialog::tabChanged);

	// Since StelDialog uses a QDialog, keep Enter from activating a button
	const QList<QPushButton*> buttons = dialog->findChildren<QPushButton*>();
	for (QPushButton* b : buttons)
		b->setAutoDefault(false);

	// The values follow the simulation time while it is running (dateChanged() and timeSyncOccurred() are not emitted then),
	// but are only recomputed when the Values tab is shown and the time has moved.
	liveTimer = new QTimer(this);
	liveTimer->setInterval(500);
	connect(liveTimer, &QTimer::timeout, this, &EOPDialog::updateLiveIfVisible);
	liveTimer->start();

	ui->progressBar->setVisible(core->isUpdatingEOP());
	if (core->isUpdatingEOP())
		ui->progressBar->setRange(0, 0);
	refreshAll();
}

// ---------------------------------------------------------------------------------------------------------------------
// Files, model and update
// ---------------------------------------------------------------------------------------------------------------------

void EOPDialog::refreshAll()
{
	if (!ui || !dialog)
		return;
	const EOPManager* mgr = core->getEOPManager();
	ui->labelFolder->setText(q_("Folder: %1").arg(mgr ? QDir::toNativeSeparators(mgr->directory()) : QString()));
	populateFilesTable();
	syncModel();
	updateSummary();
	updateDateRanges();
	showTable();
	updateLiveValues();
	updateUpdateStatus();
}

void EOPDialog::populateFilesTable()
{
	const EOPManager* mgr = core->getEOPManager();
	if (!mgr)
		return;
	populating = true;
	QTableWidget* t = ui->filesTable;
	t->setRowCount(0);

	// rows in the order of the merge (1, 2, 3...), then the files that are not merged, by name
	const QStringList order           = mgr->mergeOrder();
	QList<EOPManager::FileInfo> files = mgr->files();
	std::stable_sort(files.begin(), files.end(), [&order](const EOPManager::FileInfo& a, const EOPManager::FileInfo& b)
	{
		const int pa = order.indexOf(a.fileName), pb = order.indexOf(b.fileName);
		if ((pa < 0) != (pb < 0)) return pa >= 0; // merged files first
			return pa < pb; // both merged: merge order; both not merged: equal, so the name order is kept
	});
	const QStringList selected = mgr->selectedFiles();
	t->setRowCount(files.size());
	for (int row = 0; row < files.size(); ++row)
	{
		const EOPManager::FileInfo& fi = files.at(row);
		QTableWidgetItem* use = new QTableWidgetItem();
		use->setData(Qt::UserRole, fi.fileName);
		if (fi.valid)
		{
			use->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
			use->setCheckState(selected.contains(fi.fileName) ? Qt::Checked : Qt::Unchecked);
		}
		else
		{
			use->setFlags(Qt::ItemIsEnabled);
			use->setToolTip(fi.error);
		}
		t->setItem(row, 0, use);

		const int pos = order.indexOf(fi.fileName);
		t->setItem(row, 1, cell(pos >= 0 ? QString::number(pos + 1) : QString(), Qt::AlignHCenter | Qt::AlignVCenter));
		QTableWidgetItem* name = cell(fi.fileName);
		name->setToolTip(fi.valid ? fi.fileName : fi.error);
		t->setItem(row, 2, name);
		if (fi.valid)
		{
			t->setItem(row, 3, cell(fi.isLongTerm() ? q_("Long term") : q_("Standard")));
			t->setItem(row, 4, cell(shortModelName(fi.model)));
			t->setItem(row, 5, cell(fi.generated.isValid() ? fi.generated.toString("yyyy-MM-dd") : QStringLiteral("-")));
			t->setItem(row, 6, cell(QString("%1 %2 %3").arg(dateText(fi.firstMJD), q_("to"), dateText(fi.lastMJD))));
			t->setItem(row, 7, cell(QString::number(fi.nRecords), Qt::AlignRight | Qt::AlignVCenter));
		}
		else
		{
			t->setItem(row, 3, cell(q_("not an EOP file")));
			for (int col = 4; col < 8; ++col)
				t->setItem(row, col, cell(QString()));
		}
	}
	t->resizeColumnsToContents();
	populating = false;
}

void EOPDialog::fileItemChanged(QTableWidgetItem* item)
{
	if (populating || !item || item->column() != 0)
		return;
	const EOPManager* mgr = core->getEOPManager();
	if (!mgr)
		return;

	QStringList selection;
	for (int row = 0; row < ui->filesTable->rowCount(); ++row)
	{
		const QTableWidgetItem* it = ui->filesTable->item(row, 0);
		if (it && (it->flags() & Qt::ItemIsUserCheckable) && it->checkState() == Qt::Checked)
			selection << it->data(Qt::UserRole).toString();
	}
	if (selection.isEmpty())
	{
		lastUpdateMessage = q_("At least one file must be selected.");
		QTimer::singleShot(0, this, [this]() { populateFilesTable(); updateUpdateStatus(); });
		return;
	}

	// the default selection is stored as "no explicit selection", so that it follows the newest downloads
	QStringList def = mgr->defaultSelection();
	QStringList sorted = selection;
	def.sort();
	sorted.sort();
	const QStringList toSet = (def == sorted) ? QStringList() : selection;
	// the table is rebuilt by the signal of the core: do it once this handler is finished
	QTimer::singleShot(0, this, [this, toSet]() { core->setEOPFiles(toSet); });
}

void EOPDialog::modelChanged(int index)
{
	if (populating || index < 0)
		return;
	core->setEOPModel(ui->modelComboBox->itemData(index).toString());
}

void EOPDialog::syncModel()
{
	if (!ui || !dialog)
		return;
	const int idx = ui->modelComboBox->findData(core->getEOPModel());
	if (idx < 0)
		return;
	populating = true;
	ui->modelComboBox->setCurrentIndex(idx);
	populating = false;
}

void EOPDialog::openFolderClicked()
{
	const EOPManager* mgr = core->getEOPManager();
	if (mgr)
		QDesktopServices::openUrl(QUrl::fromLocalFile(mgr->directory()));
}

void EOPDialog::rescanClicked()
{
	core->rescanEOPFiles();
}

void EOPDialog::defaultFilesClicked()
{
	core->setEOPFiles(QStringList());
}

void EOPDialog::updateClicked()
{
	if (core->isUpdatingEOP())
		return;
	ui->pushButtonUpdate->setEnabled(false);
	ui->progressBar->setRange(0, 0);
	ui->progressBar->setVisible(true);
	lastUpdateMessage = q_("Downloading the EOP files from the IERS...");
	updateUpdateStatus();
	core->updateEOPFiles(true);
}

void EOPDialog::updateProgress(qint64 received, qint64 total)
{
	if (!ui || !dialog)
		return;
	ui->progressBar->setVisible(true);
	if (total > 0)
	{
		ui->progressBar->setRange(0, 1000);
		ui->progressBar->setValue(int(1000.0 * double(received) / double(total)));
	}
	else
		ui->progressBar->setRange(0, 0);
}

void EOPDialog::updateFinished(bool ok, const QStringList& written, const QString& message)
{
	if (!ui || !dialog)
		return;
	Q_UNUSED(written)
	ui->pushButtonUpdate->setEnabled(true);
	ui->progressBar->setVisible(false);
	lastUpdateMessage = ok ? q_("Update done: %1").arg(message) : q_("Update problem: %1").arg(message);
	updateUpdateStatus();
}

void EOPDialog::autoUpdateToggled(bool on)
{
	StelApp::immediateSave("astro/eop_auto_update", on);
}

void EOPDialog::updateUpdateStatus()
{
	const EOPManager* mgr = core->getEOPManager();
	if (!ui || !dialog || !mgr)
		return;
	QDate newest;
	const QList<EOPManager::FileInfo> files = mgr->files();
	for (const EOPManager::FileInfo& fi : files)
		if (fi.valid && fi.generated.isValid() && (!newest.isValid() || fi.generated > newest))
			newest = fi.generated;
	QString s = newest.isValid() ? q_("Newest download: %1").arg(newest.toString("yyyy-MM-dd")) : q_("No dated file in the folder.");
	if (!lastUpdateMessage.isEmpty())
		s += "\n" + lastUpdateMessage;
	ui->labelUpdateStatus->setText(s);
	ui->pushButtonUpdate->setEnabled(!core->isUpdatingEOP());
}

// ---------------------------------------------------------------------------------------------------------------------
// Fields
// ---------------------------------------------------------------------------------------------------------------------

QStringList EOPDialog::presetFields(const QString& mode) const
{
	QStringList ids;
	if (mode == "all")
	{
		for (int i = 0; i < N_FIELDS; ++i)
			ids << fieldId(i);
	}
	else if (mode == "default")	// the eight main values
		ids << "mjd" << "date" << "xp" << "yp" << "ut1" << "lod" << "dpsi" << "deps" << "dx" << "dy";
	else if (mode == "short")	// the offsets of the selected model
	{
		ids << "mjd" << "date";
		if (core->getEOPModel() == "IAU2000A")
			ids << "dx" << "dy";
		else
			ids << "dpsi" << "deps";
	}
	else if (mode == "custom")
		ids = customFields;
	return ids;
}

QStringList EOPDialog::selectedFields() const
{
	QStringList ids;
	for (int i = 0; i < fieldBoxes.size() && i < N_FIELDS; ++i)
		if (fieldBoxes.at(i)->isChecked())
			ids << fieldId(i);
	return ids;
}

void EOPDialog::setFieldCheckBoxes(const QStringList& ids)
{
	for (int i = 0; i < fieldBoxes.size() && i < N_FIELDS; ++i)
		fieldBoxes.at(i)->setChecked(ids.contains(fieldId(i)));
}

void EOPDialog::setPresetRadioButton(const QString& mode)
{
	ui->radioAll->setChecked(mode == "all");
	ui->radioDefault->setChecked(mode == "default");
	ui->radioShort->setChecked(mode == "short");
	ui->radioNone->setChecked(mode == "none");
	ui->radioCustom->setChecked(mode == "custom");
}

void EOPDialog::presetClicked(const QString& mode)
{
	fieldsMode = mode;
	StelApp::immediateSave("astro/eop_fields_mode", fieldsMode);
	setFieldCheckBoxes(presetFields(mode));
	showTable();
	updateLiveValues();
}

void EOPDialog::fieldCheckBoxClicked()
{
	customFields = selectedFields();
	fieldsMode = "custom";
	setPresetRadioButton(fieldsMode);
	StelApp::immediateSave("astro/eop_fields_mode", fieldsMode);
	StelApp::immediateSave("astro/eop_fields", customFields.join(QLatin1Char(',')));
	showTable();
	updateLiveValues();
}

// ---------------------------------------------------------------------------------------------------------------------
// Values at the date of the simulation
// ---------------------------------------------------------------------------------------------------------------------

void EOPDialog::updateLiveValues()
{
	if (!ui || !dialog)
		return;
	const double jd = core->getJD();	// JD(UT) of the simulation
	lastLiveJD = jd;
	const double mjd = jd - 2400000.5;
	ui->labelLiveHeader->setText(q_("JD(UT) = %1    MJD = %2    %3").arg(jd, 0, 'f', 5).arg(mjd, 0, 'f', 5).arg(dateText(mjd)));

	QString note = core->getUseEOP() ? QString() : q_("The EOP corrections are switched off: the values below are not applied.");
	QTableWidget* t = ui->liveTable;
	const EOPManager* mgr = core->getEOPManager();
	EOPReader::Interpolated e;
	if (!mgr || !mgr->getEOP(jd, e))
	{
		t->setRowCount(0);
		ui->labelLiveNote->setText((note + " " + q_("No EOP value is available for this date.")).trimmed());
		return;
	}

	int rows = 0;
	t->setRowCount(N_FIELDS);
	const QStringList selected = selectedFields();
	for (int i = 0; i < N_FIELDS; ++i)
	{
		if (!isValueField(i) || !selected.contains(fieldId(i)))
			continue;
		t->setItem(rows, 0, cell(QString::fromLatin1(FIELDS[i].label)));
		t->setItem(rows, 1, cell(fieldText(fieldId(i), e.rec, 5), Qt::AlignRight | Qt::AlignVCenter));
		t->setItem(rows, 2, cell(QString::fromLatin1(FIELDS[i].unit)));
		t->setItem(rows, 3, cell(statusText(fieldId(i), e)));
		++rows;
	}
	t->setRowCount(rows);
	t->resizeColumnsToContents();
	if (e.coverage == EOPReader::Coverage::AfterEnd)
		note += " " + q_("This date is after the last record: the last valid value of each EOP is kept.");
	ui->labelLiveNote->setText(note.trimmed());
}

void EOPDialog::updateLiveIfVisible()
{
	if (!ui || !dialog || !visible() || ui->tabWidget->currentWidget() != ui->tabValues || core->getJD() == lastLiveJD)
		return;
	updateLiveValues();
}

void EOPDialog::updateSummary()
{
	const EOPManager* mgr = core->getEOPManager();
	if (!ui || !dialog || !mgr)
		return;
	ui->summaryTextEdit->setPlainText(mgr->coverageText() + "\n" + mgr->lastValuesText());
	ui->reportTextEdit->setPlainText(mgr->lastReport());
}

void EOPDialog::tabChanged(int)
{
	if (ui->tabWidget->currentWidget() == ui->tabValues)
	{
		updateLiveValues();
		updateSummary();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Table of the daily records
// ---------------------------------------------------------------------------------------------------------------------

void EOPDialog::updateDateRanges()
{
	const EOPManager* mgr = core->getEOPManager();
	if (!ui || !dialog || !mgr || mgr->isEmpty())
		return;
	const QDate first = mjdToDate(mgr->reader().firstMJD());
	const QDate last = mjdToDate(mgr->reader().lastMJD());
	ui->fromDateEdit->setDateRange(first, last);
	ui->toDateEdit->setDateRange(first, last);
	if (!ui->fromDateEdit->property("eopInit").toBool())	// first time: current date +/- 5 days
	{
		ui->fromDateEdit->setProperty("eopInit", true);
		const QDate c = QDate::fromJulianDay(qint64(std::floor(core->getJD() + 0.5)));
		ui->fromDateEdit->setDate(c.addDays(-5));
		ui->toDateEdit->setDate(c.addDays(5));
	}
}

void EOPDialog::currentDateClicked()
{
	const QDate c = QDate::fromJulianDay(qint64(std::floor(core->getJD() + 0.5)));
	ui->fromDateEdit->setDate(c.addDays(-5));
	ui->toDateEdit->setDate(c.addDays(5));
	showTable();
}

void EOPDialog::showTable()
{
	const EOPManager* mgr = core->getEOPManager();
	if (!ui || !dialog || !mgr)
		return;
	QTableWidget* t = ui->dumpTable;
	t->setRowCount(0);

	// columns: the selected fields
	const QStringList ids = selectedFields();
	QStringList headers;
	for (int i = 0; i < N_FIELDS; ++i)
		if (ids.contains(fieldId(i)))
			headers << fieldLabel(i);
	t->setColumnCount(headers.size());
	t->setHorizontalHeaderLabels(headers);
	if (ids.isEmpty())
	{
		ui->labelTableInfo->setText(q_("No field is selected (Fields tab)."));
		return;
	}

	QDate d0 = ui->fromDateEdit->date(), d1 = ui->toDateEdit->date();
	if (d1 < d0)
		std::swap(d0, d1);
	const QVector<EOPReader::Record> recs = mgr->records(dateToMjd(d0), dateToMjd(d1), TABLE_MAX_ROWS + 1);
	if (recs.isEmpty())
	{
		ui->labelTableInfo->setText(q_("No record between %1 and %2.").arg(d0.toString("yyyy-MM-dd"), d1.toString("yyyy-MM-dd")));
		return;
	}
	const int n = std::min(int(recs.size()), TABLE_MAX_ROWS);
	t->setRowCount(n);
	for (int row = 0; row < n; ++row)
	{
		int col = 0;
		for (int i = 0; i < N_FIELDS; ++i)
		{
			if (!ids.contains(fieldId(i)))
				continue;
			t->setItem(row, col++, cell(fieldText(fieldId(i), recs.at(row), 0), Qt::AlignRight | Qt::AlignVCenter));
		}
	}
	t->resizeColumnsToContents();
	QString info = q_("%1 records from %2 to %3 (MJD %4 to %5)").arg(n).arg(dateText(recs.first().mjd), dateText(recs.at(n - 1).mjd))
			.arg(recs.first().mjd, 0, 'f', 0).arg(recs.at(n - 1).mjd, 0, 'f', 0);
	if (recs.size() > TABLE_MAX_ROWS)
		info += " - " + q_("only the first %1 are shown, use Save as CSV for all of them").arg(TABLE_MAX_ROWS);
	ui->labelTableInfo->setText(info);
}

void EOPDialog::saveCsv()
{
	const EOPManager* mgr = core->getEOPManager();
	if (!mgr)
		return;
	const QStringList ids = selectedFields();
	if (ids.isEmpty())
	{
		ui->labelTableInfo->setText(q_("No field is selected (Fields tab)."));
		return;
	}
	QDate d0 = ui->fromDateEdit->date(), d1 = ui->toDateEdit->date();
	if (d1 < d0)
		std::swap(d0, d1);
	const QVector<EOPReader::Record> recs = mgr->records(dateToMjd(d0), dateToMjd(d1), CSV_MAX_RECORDS);
	if (recs.isEmpty())
	{
		ui->labelTableInfo->setText(q_("No record between %1 and %2.").arg(d0.toString("yyyy-MM-dd"), d1.toString("yyyy-MM-dd")));
		return;
	}

	// by default in the user data directory of Stellarium, the folder of log.txt
	const QString defaultPath = QDir(StelFileMgr::getUserDir()).filePath(QString("eop_%1_%2.csv").arg(d0.toString("yyyyMMdd"), d1.toString("yyyyMMdd")));
	const QString path = QFileDialog::getSaveFileName(&StelMainView::getInstance(), q_("Save the EOP table"), defaultPath, "CSV (*.csv)");
	if (path.isEmpty())
		return;

	QStringList headers;
	for (int i = 0; i < N_FIELDS; ++i)
		if (ids.contains(fieldId(i)))
			headers << fieldLabel(i);
	QString text = headers.join(QLatin1Char(';')) + "\n";
	for (const EOPReader::Record& r : recs)
	{
		QStringList cells;
		for (int i = 0; i < N_FIELDS; ++i)
			if (ids.contains(fieldId(i)))
				cells << fieldText(fieldId(i), r, 0);
		text += cells.join(QLatin1Char(';')) + "\n";
	}

	QSaveFile f(path);
	if (!f.open(QIODevice::WriteOnly) || f.write(text.toUtf8()) < 0 || !f.commit())
	{
		ui->labelTableInfo->setText(q_("Cannot write %1").arg(QDir::toNativeSeparators(path)));
		return;
	}
	ui->labelTableInfo->setText(q_("%1 records saved in %2").arg(recs.size()).arg(QDir::toNativeSeparators(path)));
}
