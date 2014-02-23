/*
 * Stellarium
 * Copyright (C) 2015 Marcos Cardinot
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

#ifndef _ADDONSCANNER_HPP_
#define _ADDONSCANNER_HPP_

#include <QObject>
#include "StelAddOnMgr.hpp"
#include "StelDialog.hpp"

class Ui_addonScannerForm;

class AddOnScanner : public StelDialog
{
	Q_OBJECT
public:
	AddOnScanner(QObject* parent);
	virtual ~AddOnScanner();

	void loadAddons(QList<AddOn*>);

public slots:
	void retranslate();

private slots:
	void slotInstallCompatibles();
	void slotRemoveAll();
	void slotRemoveIncompatibles();

protected:
	Ui_addonScannerForm* ui;
        //! Initialize the dialog widgets and connect the signals/slots.
        virtual void createDialogContent();

private:
	QList<AddOn*> m_addonsList;
};

#endif // _ADDONSCANNER_HPP_
