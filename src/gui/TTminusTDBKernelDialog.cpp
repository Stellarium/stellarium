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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 * 
 * Dialog to choose the source of TT-TDB used by the JPL Horizons Delta-T algorithm
 * (SS) 2026-10-03 Modelled on CustomDeltaTEquationDialog (Copyright (C) 2013 Alexander Wolf)
*/

#include "TTminusTDBKernelDialog.hpp"
#include "ui_ttMinusTdbKernelDialog.h"

#include "Dialog.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelTranslator.hpp"
#include "StelUtils.hpp"
#include "BSPManager.hpp"

#include <QDir>
#include <QLocale>
#include <QStringList>

namespace
{
// Range of the JPL Horizons algorithm where TT-TDB is needed: from 1962-01-19 12:00 to 9999-12-30
const double JD_FIRST_USE = 2437684.0;
const double JD_LAST_USE  = 5373482.5;

QString formatDate(double jd)
{
	int y, m, d;
	StelUtils::getDateFromJulianDay(jd, &y, &m, &d);
	return QString("%1-%2-%3").arg(y).arg(m, 2, 10, QChar('0')).arg(d, 2, 10, QChar('0'));
}

// TT-TDB is at most about 1.7 ms: microseconds are the natural unit
QString formatMicroseconds(double seconds)
{
	return QString("%1%2 %3s").arg(seconds < 0. ? QChar(0x2212) : QChar('+')).arg(qAbs(seconds) * 1e6, 0, 'f', 2).arg(QChar(0x00B5));
}
}

TTminusTDBKernelDialog::TTminusTDBKernelDialog()
	: StelDialog("TTminusTDBKernel")
	, populating(false)
{
	ui = new Ui_ttMinusTdbKernelDialogForm;
	core = StelApp::getInstance().getCore();
}

TTminusTDBKernelDialog::~TTminusTDBKernelDialog()
{
	delete ui;
	ui = Q_NULLPTR;
}

void TTminusTDBKernelDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		setDescription();
		populateKernelList();
	}
}

void TTminusTDBKernelDialog::createDialogContent()
{
	ui->setupUi(dialog);
	setDescription();
	populateKernelList();

	//Signals and slots
	connect(&StelApp::getInstance(), &StelApp::languageChanged, this, &TTminusTDBKernelDialog::retranslate);
	connect(ui->titleBar, &TitleBar::closeClicked, this, &StelDialog::close);
	connect(ui->titleBar, &TitleBar::movedTo,      this, &TTminusTDBKernelDialog::handleMovedTo);

	connect(ui->kernelComboBox, qOverload<int>(&QComboBox::currentIndexChanged), this, &TTminusTDBKernelDialog::kernelSelectionChanged);
	connect(ui->pushButtonRefresh, &QPushButton::clicked, this, &TTminusTDBKernelDialog::refreshClicked);
	connect(ui->pushButtonDefault, &QPushButton::clicked, this, &TTminusTDBKernelDialog::defaultClicked);

	// The kernel can also be changed from elsewhere (scripts, property system), and the status line shows the current date
	connect(core, &StelCore::ttMinusTdbKernelChanged, this, &TTminusTDBKernelDialog::syncSelection);
	connect(core, &StelCore::dateChanged,             this, &TTminusTDBKernelDialog::updateStatus);
	connect(core, &StelCore::timeSyncOccurred,        this, &TTminusTDBKernelDialog::updateStatus);
	if (core->getBSPManager())
		connect(core->getBSPManager(), &BSPManager::kernelsChanged, this, &TTminusTDBKernelDialog::populateKernelList);
}

void TTminusTDBKernelDialog::setDescription() const
{
	ui->titleBar->setTitle(q_("TT-TDB source for the JPL Horizons algorithm"));
	ui->labelDescription->setText(q_("The JPL Horizons algorithm needs TT-TDB to compute %1T after the year 1962. It is read from an SPK kernel "
					 "(a .bsp file) that you place in the folder shown below. Only kernels that contain TT-TDB are listed.").arg(QChar(0x0394)));
	ui->pushButtonRefresh->setToolTip(q_("Scan the folder again for kernels"));
	ui->pushButtonDefault->setToolTip(q_("Use the default kernel (%1)").arg(BSPManager::defaultTTminusTDBKernel()));
	const BSPManager* mgr = core->getBSPManager();
	ui->labelFolder->setText(q_("Folder: %1").arg(mgr ? QDir::toNativeSeparators(mgr->directory()) : QString()));
}

void TTminusTDBKernelDialog::populateKernelList()
{
	if (!ui || !dialog)
		return;
	BSPManager* mgr = core->getBSPManager();

	populating = true;
	ui->kernelComboBox->clear();
	ui->kernelComboBox->addItem(q_("None (DE440T Linux file, or no TT-TDB correction)"), BSPManager::noTTminusTDBKernel());
	if (mgr)
		for (const QString& name : mgr->kernelFileNames(true))
			ui->kernelComboBox->addItem(name, name);

	// Select the kernel in use; if it is no longer in the folder keep showing it, marked as missing
	const QString current = core->getTTminusTDBKernel();
	int idx = ui->kernelComboBox->findData(current, Qt::UserRole, Qt::MatchFixedString);
	if (idx < 0)
	{
		ui->kernelComboBox->addItem(q_("%1 (not found)").arg(current), current);
		idx = ui->kernelComboBox->count() - 1;
	}
	ui->kernelComboBox->setCurrentIndex(idx);
	populating = false;

	setDescription(); // the folder line
	updateInfo();
	updateStatus();
}

void TTminusTDBKernelDialog::kernelSelectionChanged(int index)
{
	if (populating || index < 0)
		return;
	core->setTTminusTDBKernel(ui->kernelComboBox->itemData(index).toString());
	updateInfo();
	updateStatus();
}

void TTminusTDBKernelDialog::syncSelection()
{
	if (!ui || !dialog)
		return;
	const int idx = ui->kernelComboBox->findData(core->getTTminusTDBKernel(), Qt::UserRole, Qt::MatchFixedString);
	if (idx < 0)
	{
		populateKernelList(); // not in the list yet, e.g. the default kernel when it is not installed
		return;
	}
	populating = true;
	ui->kernelComboBox->setCurrentIndex(idx);
	populating = false;
	updateInfo();
	updateStatus();
}

void TTminusTDBKernelDialog::refreshClicked()
{
	// Rescan the folder; kernelsChanged() then refills the list and keeps the selection (even if the file vanished)
	if (core->getBSPManager())
		core->getBSPManager()->rescan();
}

void TTminusTDBKernelDialog::defaultClicked()
{
	core->setTTminusTDBKernel(BSPManager::defaultTTminusTDBKernel());
	syncSelection(); // also covers the case where nothing changed
}

QString TTminusTDBKernelDialog::kernelInfoText(const QString& key) const
{
	if (key == BSPManager::noTTminusTDBKernel())
		return q_("No kernel is used. TT-TDB then comes from the DE440T Linux file (linux_p1550p2650.440t) when it is installed and "
			  "the date is between the years 1550 and 2650. Outside that range, or without the file, no TT-TDB correction is applied (0 s).");

	const BSPManager* mgr = core->getBSPManager();
	const BSPManager::KernelInfo* k = mgr ? mgr->kernelInfo(key) : Q_NULLPTR;
	if (!k || !k->valid || !k->hasTTminusTDB)
		return q_("This kernel was not found in the folder below, or it contains no TT-TDB data. Until it is available, the DE440T Linux file "
			  "or zero is used.");

	QStringList lines;
	lines << q_("File: %1 (%2)").arg(k->fileName.toHtmlEscaped(), QLocale().formattedDataSize(k->sizeBytes));
	lines << q_("TT-TDB data from %1 to %2 (astronomical years; JD %3 to %4)")
		 .arg(formatDate(k->ttJdBegin), formatDate(k->ttJdEnd)).arg(k->ttJdBegin, 0, 'f', 1).arg(k->ttJdEnd, 0, 'f', 1);
	if (k->ttJdBegin <= JD_FIRST_USE && k->ttJdEnd >= JD_LAST_USE)
		lines << q_("It covers the whole range where the JPL Horizons algorithm needs TT-TDB (years 1962 to 9999).");
	else
		lines << q_("It does not cover the whole range where the JPL Horizons algorithm needs TT-TDB (years 1962 to 9999). "
			    "Outside its range the DE440T Linux file or zero is used.");
	if (k->fileName.compare(BSPManager::defaultTTminusTDBKernel(), Qt::CaseInsensitive) == 0)
		lines << q_("This is the default kernel.");
	return lines.join("<br>");
}

void TTminusTDBKernelDialog::updateInfo()
{
	if (!ui || !dialog)
		return;
	const int idx = ui->kernelComboBox->currentIndex();
	ui->labelKernelInfo->setText(idx < 0 ? QString() : kernelInfoText(ui->kernelComboBox->itemData(idx).toString()));
}

QString TTminusTDBKernelDialog::unavailableReason(const QString& key) const
{
	const BSPManager* mgr = core->getBSPManager();
	const BSPManager::KernelInfo* k = mgr ? mgr->kernelInfo(key) : Q_NULLPTR;
	if (!k)
		return q_("kernel not found");
	if (!k->valid)
		return q_("unreadable kernel");
	if (!k->hasTTminusTDB)
		return q_("no TT-TDB data");
	return q_("date outside its range");
}

void TTminusTDBKernelDialog::updateStatus()
{
	if (!ui || !dialog)
		return;

	// The simulation time is in TDB when the JPL Horizons algorithm is active
	double seconds = 0.;
	const StelCore::TTminusTDBSource source = core->getTTminusTDBSource(core->getJDE(), seconds);
	const QString selected = core->getTTminusTDBKernel();
	const bool none = (selected == BSPManager::noTTminusTDBKernel());
	QString text;

	switch (source)
	{
		case StelCore::TTminusTDBFromKernel:
			text = q_("TT-TDB at the current date: %1, from %2").arg(formatMicroseconds(seconds), selected);
			break;
		case StelCore::TTminusTDBFromDE440T:
			if (none)
				text = q_("No kernel selected. TT-TDB at the current date: %1, from the DE440T Linux file").arg(formatMicroseconds(seconds));
			else
				text = q_("%1 cannot supply TT-TDB at this date (%2). TT-TDB: %3, from the DE440T Linux file")
					.arg(selected, unavailableReason(selected), formatMicroseconds(seconds));
			break;
		case StelCore::TTminusTDBNone:
		default:
			if (none)
				text = q_("No kernel selected and no DE440T Linux file for this date: no TT-TDB correction (0 s)");
			else
				text = q_("%1 cannot supply TT-TDB at this date (%2). No DE440T Linux file for this date: no TT-TDB correction (0 s)")
					.arg(selected, unavailableReason(selected));
			break;
	}
	ui->labelStatus->setText(text);
}
