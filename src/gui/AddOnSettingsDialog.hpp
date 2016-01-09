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

#include "AddOnDialog.hpp"
#include "StelAddOnMgr.hpp"
#include "StelDialog.hpp"

class AddOnDialog;
class Ui_addonSettingsDialogForm;

//! @class AddOnSettingsDialog
class AddOnSettingsDialog : public StelDialog
{
	Q_OBJECT
public:
	AddOnSettingsDialog(AddOnDialog* addOnDialog);
	virtual ~AddOnSettingsDialog();

public slots:
	void retranslate();

protected:
	//! Initialize the dialog widgets and connect the signals/slots
	virtual void createDialogContent();
	Ui_addonSettingsDialogForm* ui;

private slots:
	void updateCatalog();
	void setAutoUpdate(bool enabled);
	void setUpdateFrequency(int index);
	void setUpdateTime(QTime time);

private:
	AddOnDialog* m_pAddOnDialog;
	StelAddOnMgr* m_pStelAddOnMgr;

	void setAboutHtml();
	void setCurrentUpdateFrequency(int days);
};

#endif // _ADDONSETTINGSDIALOG_HPP_
