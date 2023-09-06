/*
 * Stellarium Missing Stars Plug-in GUI
 * 
 * Copyright (C) 2023 Alexander Wolf
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
 
#ifndef MISSINGSTARSDIALOG_HPP
#define MISSINGSTARSDIALOG_HPP

#include <QObject>
#include "StelDialog.hpp"
#include "MissingStars.hpp"

class Ui_missingStarsDialog;
class QTimer;
class MissingStars;

//! @ingroup missingStars
class MissingStarsDialog : public StelDialog
{
	Q_OBJECT

public:
	MissingStarsDialog();
	~MissingStarsDialog() Q_DECL_OVERRIDE;

protected:
	//! Initialize the dialog widgets and connect the signals/slots
	void createDialogContent() Q_DECL_OVERRIDE;

public slots:
	void retranslate() Q_DECL_OVERRIDE;

private:
	MissingStars* msm;
	Ui_missingStarsDialog* ui;
	void setAboutHtml(void);
};

#endif // MISSINGSTARSDIALOG_HPP
