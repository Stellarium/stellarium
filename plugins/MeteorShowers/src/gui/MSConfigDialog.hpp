/*
 * Stellarium: Meteor Showers Plug-in
 * Copyright (C) 2013-2015 Marcos Cardinot
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
 
#ifndef MSCONFIGDIALOG_HPP
#define MSCONFIGDIALOG_HPP

#include "MeteorShowers.hpp"
#include "StelDialog.hpp"

class Ui_MSConfigDialog;

//! @class MSConfigDialog
//! Configuration window.
//! @author Marcos Cardinot <mcardinot@gmail.com>
//! @ingroup meteorShowers
class MSConfigDialog : public StelDialog
{
	Q_OBJECT

public:
	//! Constructor
	MSConfigDialog(MeteorShowersMgr *mgr);

	//! Destructor
	~MSConfigDialog();

	//! Initializes the components based on the current settings
	void init();

protected:
	//! Initializes the dialog and connect the signals/slots
	void createDialogContent();

public slots:
	void retranslate();

private slots:
	//! Refresh details about the last update
	void refreshUpdateTab();

	//! Refresh the color of all markers
	void refreshMarkersColor();

	//! Sets the color of the active radiant based on confirmed data.
	void setColorARC();

	//! Sets the color of the active radiant based on generic data.
	void setColorARG();

	//! Sets the color of the inactive radiant.
	void setColorIR();

	void restoreDefaults();

private:
	MeteorShowersMgr* m_mgr;
	Ui_MSConfigDialog* m_ui;

	void setAboutHtml();
};

#endif // MSCONFIGDIALOG_HPP
