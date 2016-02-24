/*
 * Stellarium
 * Copyright (C) 2014-2016 Marcos Cardinot
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

#ifndef _ADDONABOUTDIALOG_HPP_
#define _ADDONABOUTDIALOG_HPP_

#include "AddOnDialog.hpp"
#include "StelDialog.hpp"

class AddOnDialog;
class Ui_addonAboutDialogForm;

//! @class AddOnAboutDialog
class AddOnAboutDialog : public StelDialog
{
	Q_OBJECT
public:
	AddOnAboutDialog(AddOnDialog* addOnDialog);
	virtual ~AddOnAboutDialog();

public slots:
	void retranslate();

protected:
	//! Initialize the dialog widgets and connect the signals/slots
	virtual void createDialogContent();
	Ui_addonAboutDialogForm* ui;

private:
	void setAboutHtml();
};

#endif // _ADDONABOUTDIALOG_HPP_
