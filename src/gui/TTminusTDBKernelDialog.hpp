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

#ifndef TTMINUSTDBKERNELDIALOG_HPP
#define TTMINUSTDBKERNELDIALOG_HPP

#include "StelDialog.hpp"

#include <QObject>

class Ui_ttMinusTdbKernelDialogForm;
class StelCore;

//! @class TTminusTDBKernelDialog
//! Lets the user pick the SPK kernel (*.bsp file in the "ephemBSP" folder of the user data directory) that provides
//! TT - TDB to the JPL Horizons Delta-T algorithm, or "None", in which case the algorithm falls back on the DE440T
//! Linux file and then on zero. Opened from the wrench button of the Time tab of the Configuration dialog.
class TTminusTDBKernelDialog : public StelDialog
{
	Q_OBJECT

public:
	TTminusTDBKernelDialog();
	~TTminusTDBKernelDialog() override;

public slots:
	void retranslate() override;

protected:
	//! Initialize the dialog widgets and connect the signals/slots.
	void createDialogContent() override;
	Ui_ttMinusTdbKernelDialogForm *ui;

private slots:
	//! Fill the drop-down list (None + kernels containing TT-TDB) and select the kernel currently in use.
	void populateKernelList();
	//! The user picked another entry of the drop-down list.
	void kernelSelectionChanged(int index);
	//! The kernel in use was changed (by this dialog, a script or the settings): show it in the drop-down list.
	void syncSelection();
	void refreshClicked();
	void defaultClicked();
	void updateInfo();
	void updateStatus();

private:
	StelCore* core;
	bool populating;

	void setDescription() const;
	QString kernelInfoText(const QString& key) const;
	//! Short reason why the selected kernel cannot supply TT-TDB at the current date
	QString unavailableReason(const QString& key) const;
};

#endif // TTMINUSTDBKERNELDIALOG_HPP
