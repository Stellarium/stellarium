/*
 * Stellarium
 * Copyright (C) 2014 Marcos Cardinot
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

#ifndef _ADDONSETTINGSDIALOG_HPP_
#define _ADDONSETTINGSDIALOG_HPP_

#include "StelDialog.hpp"

class Ui_addonSettingsDialogForm;

class AddOnSettingsDialog : public StelDialog
{
Q_OBJECT
public:
	AddOnSettingsDialog();
	virtual ~AddOnSettingsDialog();

public slots:
	void retranslate();

protected:
	//! Initialize the dialog widgets and connect the signals/slots
	virtual void createDialogContent();
	Ui_addonSettingsDialogForm* ui;

private slots:
	void setAutoUpdate(bool enabled);
	void setUpdateFrequency(int index);
	void setUpdateTime(QTime time);

private:
	int m_iUpdateFrequency;
	int m_iUpdateTime;

	void setAboutHtml();
};

#endif // _ADDONSETTINGSDIALOG_HPP_
